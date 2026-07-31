/******************************************************************************
Copyright (C) 2026 AI Caption Plugin contributors

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
*******************************************************************************/

#ifndef AI_CAPTION_PLUGIN_SHERPA_T_ONE_CAPTION_ENGINE_H
#define AI_CAPTION_PLUGIN_SHERPA_T_ONE_CAPTION_ENGINE_H

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "CaptionEngine.h"
#include "LocalCaptionEngineSettings.h"

struct SherpaOnnxOnlineRecognizer;
struct SherpaOnnxOnlineStream;

class SherpaTOneCaptionEngine final : public CaptionEngine {
public:
    explicit SherpaTOneCaptionEngine(const LocalCaptionEngineSettings &settings);

    bool queue_audio_data(const char *data, unsigned int data_size) override;

    unsigned int preferred_sample_rate() const override;

    ~SherpaTOneCaptionEngine() override;

private:
    void worker_loop();
    void decode_audio(const std::vector<std::int16_t> &audio);
    void publish_current_result(bool final);
    void reset_utterance();

    const unsigned int num_threads;
    const std::size_t max_pending_samples;
    const std::string model_directory;

    const SherpaOnnxOnlineRecognizer *recognizer = nullptr;
    const SherpaOnnxOnlineStream *stream = nullptr;

    std::mutex queue_mutex;
    std::condition_variable queue_cv;
    std::deque<std::vector<std::int16_t>> pending_audio;
    std::size_t pending_samples = 0;
    std::atomic_bool stopping = false;
    std::thread worker;

    int caption_index = 0;
    std::string last_caption_text;
    std::chrono::steady_clock::time_point first_caption_at;
};

#endif // AI_CAPTION_PLUGIN_SHERPA_T_ONE_CAPTION_ENGINE_H
