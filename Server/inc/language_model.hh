#pragma once
#include <llama-cpp.h>
#include <mtmd.h>

#include <logger.hh>

#include <cstdint>
#include <span>
#include <string>
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
        MODEL_PATH = "/data/gemma-4-E4B-it-Q6_K.gguf",
        MODEL_MMPROJ_PATH = "/data/mmproj-BF16.gguf",
        MODEL_FORMAT_TEMPLATE = "gemma";

    uint32_t images_to_mark = 0;
    llama_pos
        n_past = 0,    // tokens processed
        n_batch = 512, // tokens to process at a time
        n_ctx = 8192;  // total tokens in context
    
    std::vector<std::pair<std::string, std::string>> message_queue_;

    llama_context_ptr context_;
    llama_model_ptr model_;
    llama_sampler_ptr sampler_;

    const llama_vocab *vocabulary_;

    mtmd::context_ptr mtmd_context_;
    mtmd::bitmaps mtmd_bitmaps_;

    mtmd::batch_ptr mtmd_batch_;

    std::string format_text_input();

    float *get_embedding(const mtmd_input_chunk *chunk, size_t chunks_len, const mtmd::input_chunks& chunks);
    void tokenize_chunk(size_t i, size_t chunks_len, const mtmd::input_chunks& chunks);
    void tokenize_inputs();

    void logits(int, std::vector<llama_token_data> &cur, llama_token_data_array &cur_p);
    llama_token sample(int);
    std::string generate_text();

    public:
    LanguageModel();
    LanguageModel(const LanguageModel&) = delete;
    LanguageModel& operator=(const LanguageModel&) = delete;

    LanguageModel(LanguageModel&&);
    LanguageModel& operator=(LanguageModel&&);

    void open();

    void load_text(std::string text);
    void load_media(const std::span<uint8_t> data);

    void test();

    void close();
    ~LanguageModel();
};
