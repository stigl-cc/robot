#include "logger.hh"
#include <llama.h>
#include <llama-cpp.h>
#include <ggml-backend.h>
#include <mtmd-helper.h>

#include <cstdint>
#include <mtmd.h>
#include <string>
#include <utility>

// llama.cpp defines its own log - define it below
#include <language_model.hh>

LanguageModel::LanguageModel() { }

LanguageModel::LanguageModel(LanguageModel&& other) {
    context_ = std::move(other.context_);
    model_ = std::move(other.model_);
    sampler_ = std::move(other.sampler_);

    vocabulary_ = std::exchange(other.vocabulary_, nullptr);

    mtmd_context_ = std::move(other.mtmd_context_);
    mtmd_bitmaps_.entries = std::move(other.mtmd_bitmaps_.entries);

    images_to_mark = std::exchange(other.images_to_mark, 0);
    n_past = std::exchange(other.n_past, 0);

    message_queue_ = std::move(other.message_queue_);
}

LanguageModel& LanguageModel::operator=(LanguageModel&& other) {
    if(this == &other)
        return *this;

    context_ = std::move(other.context_);
    model_ = std::move(other.model_);
    sampler_ = std::move(other.sampler_);

    vocabulary_ = std::exchange(other.vocabulary_, nullptr);

    mtmd_context_ = std::move(other.mtmd_context_);
    mtmd_bitmaps_.entries = std::move(other.mtmd_bitmaps_.entries);

    images_to_mark = std::exchange(other.images_to_mark, 0);
    n_past = std::exchange(other.n_past, 0);

    message_queue_ = std::move(other.message_queue_);

    return *this;
}


void LanguageModel::open() {
    ggml_backend_load_all();

    llama_model_params model_params = llama_model_default_params();
    model_params.n_gpu_layers = GPU_LAYERS;

    llama_context_params context_params = llama_context_default_params();
    context_params.n_ctx = CONTEXT_LEN;
    context_params.n_batch = CONTEXT_LEN;

    llama_sampler_chain_params sampler_params = llama_sampler_chain_default_params();

    mtmd_context_params mtmd_context_params = mtmd_context_params_default();
    mtmd_context_params.use_gpu = true;

    model_.reset(llama_model_load_from_file(MODEL_PATH.data(), model_params));
    if(!model_) {
        log_tag(LOG_ERR, "Could not open the gguf model!");
        return;
    }

    vocabulary_ = llama_model_get_vocab(model_.get());

    context_.reset(llama_init_from_model(model_.get(), context_params));
    if(!context_) {
        log_tag(LOG_ERR, "Could not create context!");
    }

    sampler_.reset(llama_sampler_chain_init(sampler_params));
    llama_sampler_chain_add(sampler_.get(), llama_sampler_init_min_p(0.05, 1));
    llama_sampler_chain_add(sampler_.get(), llama_sampler_init_temp(0.8));
    llama_sampler_chain_add(sampler_.get(), llama_sampler_init_dist(LLAMA_DEFAULT_SEED));

    mtmd_context_.reset(mtmd_init_from_file(MODEL_MMPROJ_PATH.data(), model_.get(), mtmd_context_params));

    if(!mtmd_context_) {
        log_tag(LOG_ERR, "Could not create multimodal context!");
    }
}

void LanguageModel::load_text(std::string text) {
    while(images_to_mark) {
        text += std::string(mtmd_default_marker()) + "\n";
        images_to_mark--;
    }

    message_queue_.emplace_back("user", text);
}

void LanguageModel::load_media(const std::span<uint8_t> data) {
    auto resource = mtmd_helper_bitmap_init_from_buf(mtmd_context_.get(), data.data(), data.size(), false);
    if(!resource.bitmap) {
        log_tag(LOG_ERR, "Failed to load media!");
        return;
    }

    mtmd_bitmaps_.entries.push_back(resource.bitmap);
    images_to_mark++;
}

std::string LanguageModel::format_text_input() {
    std::vector<llama_chat_message> messages;
    int alloc_size = 0;
    for( const std::pair<std::string, std::string>& msg : message_queue_) {
        messages.emplace_back(msg.first.c_str(), msg.second.c_str());
        alloc_size += msg.second.size();
    }

    int32_t len = 0;
    std::vector<char> output_buf(alloc_size * 2);

    len = llama_chat_apply_template(MODEL_FORMAT_TEMPLATE.data(), messages.data(), messages.size(), true, nullptr, 0);

    if(len < 0)
        log_tag(LOG_ERR, "Chat template not supported");

    if(static_cast<size_t>(len) > output_buf.size())
        output_buf.resize(len);

    len = llama_chat_apply_template(MODEL_FORMAT_TEMPLATE.data(), messages.data(), messages.size(), true, output_buf.data(), output_buf.size());

    if(static_cast<size_t>(len) >= output_buf.size() || len < 0)
        log_tag(LOG_ERR, "Unable to format input.");

    message_queue_.clear();
    return std::string(output_buf.data(), len);
}

void LanguageModel::tokenize_inputs() {
    int ret;
    std::string formatted_text = format_text_input();

    mtmd_input_text input_text = {
        .text = formatted_text.c_str(),
        .add_special = true,
        .parse_special = true,
    };

    mtmd::input_chunks chunks = mtmd_input_chunks_init();
    ret = mtmd_tokenize(mtmd_context_.get(),
                        chunks.ptr.get(),
                        &input_text,
                        mtmd_bitmaps_.c_ptr().data(),
                        mtmd_bitmaps_.entries.size());
    if(ret != 0) {
        log_tag(LOG_ERR, "Unable to tokenize prompt");
    }

    mtmd_bitmaps_.entries.clear();

    size_t chunks_len = mtmd_input_chunks_size(chunks.ptr.get());
    for(size_t i =0; i < chunks_len; i++) {
        const mtmd_input_chunk *chunk = mtmd_input_chunks_get(chunks.ptr.get(), i);
        mtmd_input_chunk_type chunk_type = mtmd_input_chunk_get_type(chunk);

        if(chunk_type == MTMD_INPUT_CHUNK_TYPE_TEXT) {
            llama_pos n_past_new = n_past;
            ret = mtmd_helper_eval_chunk_single(mtmd_context_.get(),
                                                context_.get(),
                                                chunk,
                                                n_past_new,
                                                0, // seq-id
                                                n_batch,
                                                i + 1 == chunks_len,
                                                &n_past_new);
            if(ret != 0) {
                log_tag(LOG_ERR, "Unable to evaluate chunk of text!");
                return;
            }

            n_past = n_past_new;
        } else {
            float *embed = nullptr;
            if(mtmd_batch_)
                embed = mtmd_batch_get_output_embd(mtmd_batch_.get(), chunk);

            if(!embed) {
                mtmd_batch_.reset(mtmd_batch_init(mtmd_context_.get()));
                ret = mtmd_batch_add_chunk(mtmd_batch_.get(), chunk);

                if(ret != 0) {
                    log_tag(LOG_ERR, "Could not add first multimedia chunk!");
                }

                for(size_t j =0; j < chunks_len; j++) {
                    const mtmd_input_chunk *next_chunk = mtmd_input_chunks_get(chunks.ptr.get(), j);
                    mtmd_input_chunk_type next_chunk_type = mtmd_input_chunk_get_type(next_chunk);
                    if(next_chunk_type == MTMD_INPUT_CHUNK_TYPE_TEXT)
                        break;

                    ret = mtmd_batch_add_chunk(mtmd_batch_.get(), next_chunk);
                    if(ret != 0)
                        break;
                }

                ret = mtmd_batch_encode(mtmd_batch_.get());
                if(ret != 0)
                    log_tag(LOG_ERR, "Failed to encode multimodal batch");
            }
        }
    }
}

void LanguageModel::close() {
    vocabulary_ = nullptr;
}

LanguageModel::~LanguageModel() {
    close();
}


void LanguageModel::test() {
    open();

    load_text("Hi, how are you doing.");
    std::string a = format_text_input();
    std::cout << std::endl << std::endl << a << std::endl << std::endl;

    close();
}
