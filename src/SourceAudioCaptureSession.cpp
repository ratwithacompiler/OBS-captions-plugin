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

#include "SourceAudioCaptureSession.h"
#include "log.c"

static void audio_captured(void *param, obs_source_t *source, const struct audio_data *audio, bool muted) {
    auto session = reinterpret_cast<SourceAudioCaptureSession *>(param);
    if (session)
        session->audio_capture_cb(source, audio, muted);
}

static void state_changed_fwder(void *param, calldata_t *calldata) {
    auto session = reinterpret_cast<SourceAudioCaptureSession *>(param);
    if (session)
        session->state_changed_check();
}


SourceAudioCaptureSession::SourceAudioCaptureSession(
        obs_source_t *audio_source_arg,
        obs_source_t *muting_source_arg,
        audio_chunk_data_cb audio_data_cb,
        audio_capture_status_change_cb status_change_cb,
        resample_info resample_to,
        source_capture_config muted_handling,
        bool send_startup_change_signal,
        int id
) :
        audio_source(audio_source_arg),
        muting_source(muting_source_arg),
        on_caption_cb_handle(audio_data_cb),
        on_status_cb_handle(status_change_cb),
        muted_handling(muted_handling),
        use_muting_cb_signal(true),
        bytes_per_channel(get_audio_bytes_per_channel(resample_to.format)),
        resampler(nullptr),
        id(id) {
    debug_log("SourceAudioCaptureSession()");

    const struct audio_output_info *obs_audio = audio_output_get_info(obs_get_audio());
    if (!obs_audio)
        throw std::string("Failed to get OBS audio info");

    if (!audio_source)
        throw std::string("No audio capture source");

    if (!bytes_per_channel)
        throw std::string("Failed to get frame bytes size per channel");

    if (obs_audio->samples_per_sec != resample_to.samples_per_sec
        || obs_audio->format != resample_to.format
        || obs_audio->speakers != resample_to.speakers) {
        resample_info src = {
                obs_audio->samples_per_sec,
                obs_audio->format,
                obs_audio->speakers
        };

        info_log("creating new resampler (%d, %d, %d) -> (%d, %d, %d)",
                 src.samples_per_sec, src.format, src.speakers,
                 resample_to.samples_per_sec, resample_to.format, resample_to.speakers);

        // All audio data from sources gets resampled to the main OBS audio settings before being given to the
        // callbacks so it should be safe to assume that the src info for the resampler should never change since
        // OBS audio settings don't seem to be able to be changed while running and require a OBS restart.
        resampler = audio_resampler_create(&resample_to, &src);
        if (!resampler)
            throw std::string("Failed to create audio resampler");
    }

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
    if (send_startup_change_signal)
        state_changed_check(true);
}

void SourceAudioCaptureSession::state_changed_check(bool always_signal) {
    audio_source_capture_status new_status = check_source_status();
    if (!always_signal && new_status == capture_status)
        return;

    debug_log("SourceAudioCaptureSession %d status changed %s %d", id, obs_source_get_name(muting_source), new_status);
    capture_status = new_status;
    {
        std::lock_guard<std::recursive_mutex> lock(on_status_cb_handle.mutex);
        if (on_status_cb_handle.callback_fn)
            on_status_cb_handle.callback_fn(id, new_status);
    }

//    info_log("");
//    info_log("____%s", obs_source_get_name(source));
//    info_log("is_active: %d", is_active);
//    info_log("is_showing: %d", is_showing);
//    info_log("is_muted: %d", is_muted);
//    info_log("is_enabled: %d", is_enabled);
}


audio_source_capture_status SourceAudioCaptureSession::check_source_status() {
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

void SourceAudioCaptureSession::audio_capture_cb(obs_source_t *source, const struct audio_data *audio, bool muted) {
    if (!on_caption_cb_handle.callback_fn)
        return;

    if (!audio || !audio->frames)
        return;

    if (muted && !use_muting_cb_signal) {
        muted = false;
//        info_log("ignoring muted signal because other caption base");
    }


    if (muted || capture_status != AUDIO_SOURCE_CAPTURING) {
        if (muted_handling == MUTED_SOURCE_DISCARD_WHEN_MUTED)
            return;

        if (muted_handling == MUTED_SOURCE_REPLACE_WITH_ZERO) {
            const unsigned int size = audio->frames * bytes_per_channel;
            uint8_t *buffer = new uint8_t[size];
            memset(buffer, 0, size);

            {
                std::lock_guard<std::recursive_mutex> lock(on_caption_cb_handle.mutex);
                if (on_caption_cb_handle.callback_fn)
                    on_caption_cb_handle.callback_fn(id, buffer, size);
            }

            delete[] buffer;
            return;

        }
        if (muted_handling != MUTED_SOURCE_STILL_CAPTURE)
            return; // unknown val, capture not allowed explicitliy do nothing
    }

    if (!resampler) {
        // correct format already, no need to resample;
        unsigned int size = audio->frames * bytes_per_channel;
        {
            std::lock_guard<std::recursive_mutex> lock(on_caption_cb_handle.mutex);
            if (on_caption_cb_handle.callback_fn)
                on_caption_cb_handle.callback_fn(id, audio->data[0], size);
        }
    } else {
        uint8_t *out[MAX_AV_PLANES];
        memset(out, 0, sizeof(out));

        uint32_t out_frames;
        uint64_t ts_offset;

        // resamplers just write pointers to it's internal buffer to out[] for each channel
        // so no need to alloc/free any specific buffer here for the audio data.
        // Cb's are responsible for not keeping pointer beyond cb.
        bool success = audio_resampler_resample(resampler, out, &out_frames, &ts_offset,
                                                (const uint8_t *const *) audio->data, audio->frames);

        if (!success || !out[0]) {
            warn_log("failed resampling audio data");
            return;
        }

        unsigned int size = out_frames * bytes_per_channel;
        {
            std::lock_guard<std::recursive_mutex> lock(on_caption_cb_handle.mutex);
            if (on_caption_cb_handle.callback_fn)
                on_caption_cb_handle.callback_fn(id, out[0], size);
        }
    }
}

SourceAudioCaptureSession::~SourceAudioCaptureSession() {
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

    debug_log("SourceAudioCaptureSessionession() deaded");
}

audio_source_capture_status SourceAudioCaptureSession::get_current_capture_status() {
    return capture_status;
}
