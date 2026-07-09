#pragma once
#include <llama.h>
#include <logger.hh>

#include <cstdint>
#include <string_view>

class LanguageModel {
    private:
    static constexpr std::string_view LOG_TAG = "LanguageModel";
    static constexpr int32_t
        GPU_LAYERS = 99;
    static constexpr uint32_t
        CONTEXT_LEN = 32767;

    // TODO: Replace with some kind of more universal model path system
    static constexpr std::string_view
        modelPath_ = "/data/gemma-4-E4B-it-Q6_K.gguf",
        modelMmprojPath_ = "/data/mmproj-BF16.gguf";

    llama_context *context_;
    llama_model *model_;
    llama_sampler *sampler_;
    const llama_vocab *vocabulary_;

    public:
    LanguageModel();
    LanguageModel(const LanguageModel&) = delete;
    LanguageModel& operator=(const LanguageModel&) = delete;

    LanguageModel(LanguageModel&&);
    LanguageModel& operator=(LanguageModel&&);
    
    void open();

    void close();
    ~LanguageModel();
};
