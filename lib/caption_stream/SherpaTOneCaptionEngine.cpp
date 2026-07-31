/******************************************************************************
Copyright (C) 2026 AI Caption Plugin contributors

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
*******************************************************************************/

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include "SherpaTOneCaptionEngine.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <stdexcept>
#include <utility>

#include <sherpa-onnx/c-api/c-api.h>

namespace {
constexpr unsigned int kSampleRate = 8000;
constexpr unsigned int kFeatureDimension = 80;
constexpr float kEndpointTrailingSilenceSeconds = 1.2F;
constexpr float kEndpointMaximumUtteranceSeconds = 20.0F;

std::string checked_model_file(const std::string &directory, const char *filename) {
    const std::filesystem::path path = std::filesystem::path(directory) / filename;
    if (!std::filesystem::is_regular_file(path))
        throw std::runtime_error("Local Russian model is missing '" + path.string() + "'. Run Install-AICaptionPlugin.ps1 again.");
    return path.string();
}
}

SherpaTOneCaptionEngine::SherpaTOneCaptionEngine(const LocalCaptionEngineSettings &settings)
        : num_threads(std::max(1U, settings.num_threads)),
          max_pending_samples(kSampleRate * std::max(1U, settings.max_pending_audio_ms) / 1000U),
          model_directory(settings.model_directory),
          first_caption_at(std::chrono::steady_clock::now()) {
    if (model_directory.empty())
        throw std::runtime_error("Local Russian model directory is unavailable. Run Install-AICaptionPlugin.ps1 again.");

    const std::string model = checked_model_file(model_directory, "model.onnx");
    const std::string tokens = checked_model_file(model_directory, "tokens.txt");

    SherpaOnnxOnlineRecognizerConfig config{};
    config.feat_config.sample_rate = static_cast<int32_t>(kSampleRate);
    config.feat_config.feature_dim = static_cast<int32_t>(kFeatureDimension);
    config.model_config.t_one_ctc.model = model.c_str();
    config.model_config.tokens = tokens.c_str();
    config.model_config.provider = "cpu";
    config.model_config.num_threads = static_cast<int32_t>(num_threads);
    config.decoding_method = "greedy_search";
    config.enable_endpoint = 1;
    config.rule1_min_trailing_silence = 2.4F;
    config.rule2_min_trailing_silence = kEndpointTrailingSilenceSeconds;
    config.rule3_min_utterance_length = kEndpointMaximumUtteranceSeconds;

    recognizer = SherpaOnnxCreateOnlineRecognizer(&config);
    if (!recognizer)
        throw std::runtime_error("Unable to start the local Russian recognition model.");

    stream = SherpaOnnxCreateOnlineStream(recognizer);
    if (!stream) {
        SherpaOnnxDestroyOnlineRecognizer(recognizer);
        recognizer = nullptr;
        throw std::runtime_error("Unable to create a local recognition stream.");
    }

    worker = std::thread(&SherpaTOneCaptionEngine::worker_loop, this);
}

bool SherpaTOneCaptionEngine::queue_audio_data(const char *data, unsigned int data_size) {
    if (!data || data_size < sizeof(std::int16_t) || stopping.load())
        return false;

    const std::size_t sample_count = data_size / sizeof(std::int16_t);
    std::vector<std::int16_t> audio(sample_count);
    std::memcpy(audio.data(), data, sample_count * sizeof(std::int16_t));

    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        while (!pending_audio.empty() && pending_samples + sample_count > max_pending_samples) {
            pending_samples -= pending_audio.front().size();
            pending_audio.pop_front();
        }

        if (sample_count > max_pending_samples)
            audio.erase(audio.begin(), audio.end() - static_cast<std::ptrdiff_t>(max_pending_samples));

        pending_samples += audio.size();
        pending_audio.push_back(std::move(audio));
    }
    queue_cv.notify_one();
    return true;
}

unsigned int SherpaTOneCaptionEngine::preferred_sample_rate() const {
    return kSampleRate;
}

SherpaTOneCaptionEngine::~SherpaTOneCaptionEngine() {
    stopping.store(true);
    queue_cv.notify_all();
    if (worker.joinable())
        worker.join();

    if (stream)
        SherpaOnnxDestroyOnlineStream(stream);
    if (recognizer)
        SherpaOnnxDestroyOnlineRecognizer(recognizer);
}

void SherpaTOneCaptionEngine::worker_loop() {
#ifdef _WIN32
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
#endif

    while (!stopping.load()) {
        std::vector<std::int16_t> audio;
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            queue_cv.wait(lock, [this] { return stopping.load() || !pending_audio.empty(); });
            if (stopping.load())
                break;

            audio = std::move(pending_audio.front());
            pending_samples -= audio.size();
            pending_audio.pop_front();
        }
        decode_audio(audio);
    }
}

void SherpaTOneCaptionEngine::decode_audio(const std::vector<std::int16_t> &audio) {
    if (audio.empty())
        return;

    std::vector<float> samples(audio.size());
    std::transform(audio.begin(), audio.end(), samples.begin(), [](std::int16_t sample) {
        return static_cast<float>(sample) / 32768.0F;
    });

    SherpaOnnxOnlineStreamAcceptWaveform(
            stream,
            static_cast<int32_t>(kSampleRate),
            samples.data(),
            static_cast<int32_t>(samples.size()));

    bool decoded = false;
    while (SherpaOnnxIsOnlineStreamReady(recognizer, stream)) {
        SherpaOnnxDecodeOnlineStream(recognizer, stream);
        decoded = true;
    }

    if (decoded)
        publish_current_result(false);

    if (SherpaOnnxOnlineStreamIsEndpoint(recognizer, stream)) {
        publish_current_result(true);
        reset_utterance();
    }
}

void SherpaTOneCaptionEngine::publish_current_result(bool final) {
    const SherpaOnnxOnlineRecognizerResult *result = SherpaOnnxGetOnlineStreamResult(recognizer, stream);
    if (!result)
        return;

    const std::string text = result->text ? result->text : "";
    SherpaOnnxDestroyOnlineRecognizerResult(result);

    if (text.empty())
        return;
    if (!final && text == last_caption_text)
        return;

    const auto now = std::chrono::steady_clock::now();
    if (last_caption_text.empty())
        first_caption_at = now;

    CaptionResult caption_result(
            caption_index,
            final,
            final ? 1.0 : 0.7,
            text,
            "local-sherpa-t-one",
            first_caption_at,
            now);
    last_caption_text = text;
    std::lock_guard<std::recursive_mutex> lock(on_caption_cb_handle.mutex);
    if (on_caption_cb_handle.callback_fn)
        on_caption_cb_handle.callback_fn(caption_result, false);
}

void SherpaTOneCaptionEngine::reset_utterance() {
    SherpaOnnxOnlineStreamReset(recognizer, stream);
    last_caption_text.clear();
    first_caption_at = std::chrono::steady_clock::now();
    ++caption_index;
}
