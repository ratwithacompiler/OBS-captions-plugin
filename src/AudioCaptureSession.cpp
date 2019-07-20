/******************************************************************************
Copyright (C) 2019 by <rat.with.a.compiler@gmail.com>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include <utility>

#include <utility>
#include <util/platform.h>

#include "AudioCaptureSession.h"
#include "log.c"

static unsigned int capture_count = 0;

static void audio_captured(void *param, obs_source_t *source, const struct audio_data *audio, bool muted) {
    auto session = reinterpret_cast<AudioCaptureSession *>(param);
    session->audio_capture_cb(source, audio, muted);
}

static void state_changed_fwder(void *param, calldata_t *calldata) {
    auto session = reinterpret_cast<AudioCaptureSession *>(param);
    session->state_changed_check();
}

AudioCaptureSession::AudioCaptureSession(
        obs_source_t *audio_source_arg,
        obs_source_t *muting_source_arg,
        audio_chunk_data_cb audio_data_cb,
        audio_capture_status_change_cb status_change_cb,
        resample_info resample_to,
        source_capture_config muted_handling
) :
        audio_source(audio_source_arg),
        muting_source(muting_source_arg),
        on_caption_cb_handle(audio_data_cb),
        on_status_cb_handle(status_change_cb),
        muted_handling(muted_handling),
        use_muting_cb_signal(true) {
    info_log("AudioCaptureSession()");

    obs_audio_info backend_audio_settings;
    if (!obs_get_audio_info(&backend_audio_settings))
        throw std::string("Failed to get OBS audio info");

    if (!audio_source)
        throw std::string("No audio capture source");

    resample_info src = {
            backend_audio_settings.samples_per_sec,
            AUDIO_FORMAT_FLOAT_PLANAR,
            backend_audio_settings.speakers
    };

    resampler = audio_resampler_create(&resample_to, &src);
    if (!resampler)
        throw std::string("Failed to create audio resampler");

    const char *name = obs_source_get_name(audio_source);
    info_log("source %s active: %d", name, obs_source_active(audio_source));

    if (muting_source) {
        use_muting_cb_signal = false;

        const char *muting_name = obs_source_get_name(muting_source);
        info_log("using separarte muting source %s active: %d", muting_name, obs_source_active(muting_source));
    } else {
        muting_source = audio_source;
        info_log("using direct source %s active: %d", name, obs_source_active(audio_source));
    }

    if (!muting_source)
        throw std::string("No muting source");

    signal_handler_connect(obs_source_get_signal_handler(muting_source), "enable", state_changed_fwder, this);

    signal_handler_connect(obs_source_get_signal_handler(muting_source), "mute", state_changed_fwder, this);

    signal_handler_connect(obs_source_get_signal_handler(muting_source), "hide", state_changed_fwder, this);
    signal_handler_connect(obs_source_get_signal_handler(muting_source), "show", state_changed_fwder, this);

    signal_handler_connect(obs_source_get_signal_handler(muting_source), "activate", state_changed_fwder, this);
    signal_handler_connect(obs_source_get_signal_handler(muting_source), "deactivate", state_changed_fwder, this);

    obs_source_add_audio_capture_callback(audio_source, audio_captured, this);

//    const bool do_capture = should_send_data();
    capture_status = check_source_status();
    state_changed_check(true);
}

void AudioCaptureSession::state_changed_check(bool always_signal) {
    audio_source_capture_status new_status = check_source_status();
    if (!always_signal && new_status == capture_status)
        return;

    info_log("Capture status changed %s %d", obs_source_get_name(muting_source), new_status);
    capture_status = new_status;
    {
        std::lock_guard<std::recursive_mutex> lock(on_status_cb_handle.mutex);
        if (on_status_cb_handle.callback_fn)
            on_status_cb_handle.callback_fn(new_status);
    }

//    info_log("");
//    info_log("____%s", obs_source_get_name(source));
//    info_log("is_active: %d", is_active);
//    info_log("is_showing: %d", is_showing);
//    info_log("is_muted: %d", is_muted);
//    info_log("is_enabled: %d", is_enabled);
}


audio_source_capture_status AudioCaptureSession::check_source_status() {
    const bool is_muted = obs_source_muted(muting_source);
    const bool is_active = obs_source_active(muting_source);
    const bool is_showing = obs_source_showing(muting_source);
    const bool is_enabled = obs_source_enabled(muting_source);

    if (!is_active || !is_showing || !is_enabled)
        return AUDIO_SOURCE_NOT_STREAMED;

    if (is_muted)
        return AUDIO_SOURCE_MUTED;

//    return is_active && is_showing && is_enabled && !is_muted;
    return AUDIO_SOURCE_CAPTURING;
}

void AudioCaptureSession::audio_capture_cb(obs_source_t *source, const struct audio_data *audio, bool muted) {
    capture_count++;
    if (capture_count % 100 == 0) {
//        printf("audio_capture_cb %d %d\n", capture_count, muted);
    }
    if (!on_caption_cb_handle.callback_fn)
        return;

    if (!audio || !audio->frames)
        return;

    uint8_t *out[MAX_AV_PLANES];
    uint32_t out_frames;
    uint64_t ts_offset;

    if (muted && !use_muting_cb_signal) {
        muted = false;
//        info_log("ignoring muted signal because other caption base");
    }


    if (muted || capture_status != AUDIO_SOURCE_CAPTURING) {
        if (muted_handling == MUTED_SOURCE_DISCARD_WHEN_MUTED)
            return;

        if (muted_handling == MUTED_SOURCE_REPLACE_WITH_ZERO) {
            const unsigned int size = audio->frames * FRAME_SIZE;
            uint8_t *buffer = new uint8_t[size];
            memset(buffer, 0, size);

//            info_log("sending zero data");
            {
                std::lock_guard<std::recursive_mutex> lock(on_caption_cb_handle.mutex);
                if (on_caption_cb_handle.callback_fn)
                    on_caption_cb_handle.callback_fn(buffer, size);
            }

            delete[] buffer;
            return;

        }
        if (muted_handling != MUTED_SOURCE_STILL_CAPTURE)
            return; // unknown val, capture not allowed explicitliy do nothing
    }

    bool success = audio_resampler_resample(resampler, out, &out_frames, &ts_offset,
                                            (const uint8_t *const *) audio->data, audio->frames);

    if (!success || !out[0]) {
        warn_log("failed resampling audio data");
        return;
    }
    unsigned int size = out_frames * FRAME_SIZE;
    {
        std::lock_guard<std::recursive_mutex> lock(on_caption_cb_handle.mutex);
        if (on_caption_cb_handle.callback_fn)
            on_caption_cb_handle.callback_fn(out[0], size);
    }
}

AudioCaptureSession::~AudioCaptureSession() {
    on_caption_cb_handle.clear();
    on_status_cb_handle.clear();

    obs_source_remove_audio_capture_callback(audio_source, audio_captured, this);

    signal_handler_disconnect(obs_source_get_signal_handler(muting_source), "enable", state_changed_fwder, this);

    signal_handler_disconnect(obs_source_get_signal_handler(muting_source), "mute", state_changed_fwder, this);

    signal_handler_disconnect(obs_source_get_signal_handler(muting_source), "hide", state_changed_fwder, this);
    signal_handler_disconnect(obs_source_get_signal_handler(muting_source), "show", state_changed_fwder, this);

    signal_handler_disconnect(obs_source_get_signal_handler(muting_source), "activate", state_changed_fwder, this);
    signal_handler_disconnect(obs_source_get_signal_handler(muting_source), "deactivate", state_changed_fwder, this);

    audio_resampler_destroy(resampler);

    info_log("~AudioCaptureSession() deaded");
}
