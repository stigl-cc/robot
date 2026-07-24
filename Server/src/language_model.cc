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

std::string LanguageModel::format_text_input() {
    std::vector<llama_chat_message> messages;
    int alloc_size = 0;
    for(const std::pair<std::string, std::string>& msg : message_queue_) {
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

    if(static_cast<size_t>(len) > output_buf.size() || len < 0)
        log_tag(LOG_ERR, "Unable to format input.");

    message_queue_.clear();
    return std::string(output_buf.data(), len);
}

float* LanguageModel::get_embedding(const mtmd_input_chunk *chunk, size_t chunks_len, const mtmd::input_chunks& chunks) {
    int ret;

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
            log_tag(LOG_ERR, "Failed to encode multimodal batch!");

        embed = mtmd_batch_get_output_embd(mtmd_batch_.get(), chunk);
    }

    if(embed == nullptr) {
        log_tag(LOG_ERR, "Failed to get an embedding!");
    }

    return embed;
}

void LanguageModel::tokenize_chunk(size_t i, size_t chunks_len, const mtmd::input_chunks& chunks) {
    int ret;
    llama_pos n_past_new;

    const mtmd_input_chunk *chunk = mtmd_input_chunks_get(chunks.ptr.get(), i);
    mtmd_input_chunk_type chunk_type = mtmd_input_chunk_get_type(chunk);

    if(chunk_type == MTMD_INPUT_CHUNK_TYPE_TEXT) {
        ret = mtmd_helper_eval_chunk_single(mtmd_context_.get(), context_.get(),
                                            chunk,
                                            n_past, 0, n_batch, /*i + 1 == chunks_len*/true,
                                            &n_past_new);
    } else {
        float *embed = get_embedding(chunk, chunks_len, chunks);
        ret = mtmd_helper_decode_image_chunk(mtmd_context_.get(), context_.get(),
                                             chunk, embed,
                                             n_past, 0, n_batch, &n_past_new,
                                             nullptr, nullptr);
    }

    if(ret != 0) {
        log_tag(LOG_ERR, "Unable to evaluate a chunk!");
            return;
    }
    n_past = n_past_new;
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
    for(size_t i =0; i < chunks_len; i++)
        tokenize_chunk(i, chunks_len, chunks);
}

void LanguageModel::logits(int i, std::vector<llama_token_data> &cur, llama_token_data_array &cur_p) {
    const float
        *probs = llama_get_sampled_probs_ith(context_.get(), i),
        *logits = llama_get_sampled_logits_ith(context_.get(), i);
    const llama_token *ids = llama_get_sampled_candidates_ith(context_.get(), i);

    int vocabulary_len = llama_vocab_n_tokens(vocabulary_);

    if(probs) {
        uint32_t probs_len = llama_get_sampled_probs_count_ith(context_.get(), i);
        cur.resize(probs_len);
        for(uint32_t i = 0; i < probs_len; i++)
            cur[i] = llama_token_data{ ids[i], logits[i], probs[i] };
    } else if(logits) {
        uint32_t logits_len = llama_get_sampled_logits_count_ith(context_.get(), i);
        cur.resize(logits_len);
        for(uint32_t i = 0; i < logits_len; i++)
            cur[i] = llama_token_data{ ids[i], logits[i], 0.0000f };

    } else {
        const float *logits = llama_get_logits_ith(context_.get(), i);
        if(logits == nullptr)
            log_tag(LOG_ERR, "Failed to get logits!");

        cur.resize(vocabulary_len);
        for(llama_token id = 0; id < vocabulary_len; id++)
            cur[id] = llama_token_data{ id, logits[id], 0.0000f };
    }

    cur_p = { cur.data(), cur.size(), -1, false };
}

llama_token LanguageModel::sample(int index) {
    std::vector<llama_token_data> cur;
    llama_token_data_array cur_p;

    llama_synchronize(context_.get());

    llama_token token_id = LLAMA_TOKEN_NULL;

    logits(index, cur, cur_p);
    token_id = llama_get_sampled_token_ith(context_.get(), index);

    if(token_id != LLAMA_TOKEN_NULL) {
        for(size_t i =0; i < cur_p.size; i++) {
            if(cur_p.data[i].id == token_id) {
                cur_p.selected = i;
                break;
            }
        }

        return token_id;
    }


    llama_sampler_apply(sampler_.get(), &cur_p);
    return cur_p.data[cur_p.selected].id;
}

std::string LanguageModel::generate_text() {
    std::string output;
    std::vector<llama_token> generated_tokens;
    int max_predict = 512;
    for(int i =0; i < max_predict; i++) {
        if(i >= max_predict) {
            break;
        }

        llama_token token_id = sample(-1);
        if(llama_vocab_is_eog(vocabulary_, token_id)) break;

        generated_tokens.push_back(token_id);
        llama_sampler_accept(sampler_.get(), token_id);

        llama_batch batch = llama_batch_get_one(&token_id, 1);
        if(llama_decode(context_.get(), batch) != 0) {
            log_tag(LOG_ERR, "Failed to decode token!");
            break;
        }
        n_past++;

        std::string piece;
        piece.resize(16);

        int len = llama_token_to_piece(vocabulary_, token_id, &piece[0], piece.size(), 0, true);
        if(len < 0) {
            piece.resize(-len);
            int check = llama_token_to_piece(vocabulary_, token_id, &piece[0], piece.size(), 0, true);
            if(check != -len) {
                log_tag(LOG_ERR, "Unable to convert token to piece!");
            } else {
                piece.resize(check);
            }
        }

        output.append(piece);
    }

    //
    std::string detokenized;
    detokenized.resize(generated_tokens.size());
    int32_t detokenized_len = llama_detokenize(vocabulary_, generated_tokens.data(), static_cast<int32_t>(generated_tokens.size()), &detokenized[0], static_cast<int32_t>(detokenized.size()), false, true);
    if (detokenized_len < 0) {
        detokenized.resize(-detokenized_len);
        detokenized_len = llama_detokenize(vocabulary_, generated_tokens.data(), static_cast<int32_t>(generated_tokens.size()), &detokenized[0], static_cast<int32_t>(detokenized.size()), false, true);
        GGML_ASSERT(detokenized_len <= static_cast<int32_t>(detokenized.size()));  // whitespace trimming is performed after per-token detokenization
    }

    detokenized.resize(detokenized_len);
    message_queue_.emplace_back("assistant", detokenized);

    return output;
}

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

void LanguageModel::close() {
    vocabulary_ = nullptr;
}

LanguageModel::~LanguageModel() {
    close();
}


void LanguageModel::test() {
    open();

    load_text("How would you describe the effect of donald trump on the economy, say 50events, in the format {actions: [ {\"type\":\"positive\", name: \"increase tarrifs\"} ]}");
    //std::string a = format_text_input();
    //std::cout << std::endl << std::endl << a << std::endl << std::endl;

    tokenize_inputs();
    std::cout << generate_text() << "\n";

    close();
}
