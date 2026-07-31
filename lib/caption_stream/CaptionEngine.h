#ifndef AI_CAPTION_PLUGIN_CAPTION_ENGINE_H
#define AI_CAPTION_PLUGIN_CAPTION_ENGINE_H

#include <functional>

#include "CaptionResult.h"
#include "ThreadsaferCallback.h"

using CaptionEngineCallback = std::function<void(const CaptionResult &caption_result, bool interrupted)>;

class CaptionEngine {
public:
    ThreadsaferCallback<CaptionEngineCallback> on_caption_cb_handle;

    virtual bool queue_audio_data(const char *data, unsigned int data_size) = 0;

    virtual unsigned int preferred_sample_rate() const {
        return 16000;
    }

    virtual ~CaptionEngine() {
        on_caption_cb_handle.clear();
    }
};

#endif // AI_CAPTION_PLUGIN_CAPTION_ENGINE_H
