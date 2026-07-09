#include <language_model.hh>

#include <ggml-backend.h>
#include <llama-cpp.h>
#include <llama.h>
#include <utility>

LanguageModel::LanguageModel() : context_(nullptr),
                                 model_(nullptr),
                                 sampler_(nullptr) { }

LanguageModel::LanguageModel(LanguageModel&& other) {
    context_ = std::exchange(other.context_, nullptr);
    model_ = std::exchange(other.model_, nullptr);
    sampler_ = std::exchange(other.sampler_, nullptr);
}

LanguageModel& LanguageModel::operator=(LanguageModel&& other) {
    if(this == &other)
        return *this;

    context_ = std::exchange(other.context_, nullptr);
    model_ = std::exchange(other.model_, nullptr);
    sampler_ = std::exchange(other.sampler_, nullptr);
    vocabulary_ = std::exchange(other.vocabulary_, nullptr);
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

    model_ = llama_model_load_from_file(modelPath_.data(), model_params);
    if(!model_) {
        log_tag(LOG_ERR, "Could not open the gguf model!");
        return;
    }

    vocabulary_ = llama_model_get_vocab(model_);

    context_ = llama_init_from_model(model_, context_params);
    if(!context_) {
        log_tag(LOG_ERR, "Could not create context!");
    }

    sampler_ = llama_sampler_chain_init(sampler_params);
    llama_sampler_chain_add(sampler_, llama_sampler_init_min_p(0.05, 1));
    llama_sampler_chain_add(sampler_, llama_sampler_init_temp(0.8));
    llama_sampler_chain_add(sampler_, llama_sampler_init_dist(LLAMA_DEFAULT_SEED));
}

void LanguageModel::close() {
    if(model_) {
        llama_sampler_free(sampler_);
        llama_free(context_);
        llama_model_free(model_);
    }
}

LanguageModel::~LanguageModel() {
    close();
}
