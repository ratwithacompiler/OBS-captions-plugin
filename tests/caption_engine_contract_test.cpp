#include <cassert>
#include <string>

#include "ContinuousCaptions.h"
#include "CaptionEngineFactory.h"

class FakeCaptionEngine final : public CaptionEngine {
public:
    bool queue_audio_data(const char *data, unsigned int data_size) override {
        CaptionResult result;
        result.final = true;
        result.caption_text.assign(data, data_size);

        std::lock_guard<std::recursive_mutex> lock(on_caption_cb_handle.mutex);
        if (on_caption_cb_handle.callback_fn) {
            on_caption_cb_handle.callback_fn(result, false);
        }
        return true;
    }
};

int main() {
    FakeCaptionEngine engine;
    std::string received;
    bool interrupted = true;

    engine.on_caption_cb_handle.set([&](const CaptionResult &result, bool was_interrupted) {
        received = result.caption_text;
        interrupted = was_interrupted;
    });

    assert(engine.queue_audio_data("test", 4));
    assert(received == "test");
    assert(!interrupted);

    const CaptionStreamSettings stream_settings(
            5000,
            5000,
            180000,
            50,
            40,
            "en-US",
            0,
            "");
    const ContinuousCaptionStreamSettings engine_settings(280, 2, 10, stream_settings);

    const CaptionEngineCreationResult google_engine = CaptionEngineFactory::create(
            CaptionEngineType::GoogleHttpLegacy,
            engine_settings);
    assert(google_engine.succeeded());

    LocalCaptionEngineSettings local_settings;
    local_settings.model_directory = "this-model-directory-does-not-exist";
    const CaptionEngineCreationResult unavailable_local_engine = CaptionEngineFactory::create(
            CaptionEngineType::LocalSherpaTOne,
            engine_settings,
            local_settings);
    assert(!unavailable_local_engine.succeeded());
    assert(unavailable_local_engine.status == CaptionEngineCreationStatus::ConstructionFailed);

    const CaptionEngineCreationResult unsupported_engine = CaptionEngineFactory::create(
            static_cast<CaptionEngineType>(-1),
            engine_settings);
    assert(!unsupported_engine.succeeded());
    assert(unsupported_engine.status == CaptionEngineCreationStatus::UnsupportedEngine);
    return 0;
}
