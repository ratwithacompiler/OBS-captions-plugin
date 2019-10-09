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

#pragma once

#include <obs.h>
#include <obs-module.h>
#include <obs-frontend-api.h>
#include <media-io/audio-resampler.h>
#include <obs.hpp>
#include <functional>
#include <mutex>
#include <lib/caption_stream/ThreadsaferCallback.h>


enum audio_source_capture_status {
    AUDIO_SOURCE_CAPTURING = 1,
    AUDIO_SOURCE_MUTED = 2,
    AUDIO_SOURCE_NOT_STREAMED = 3,
};

enum source_capture_config {
    MUTED_SOURCE_DISCARD_WHEN_MUTED,
    MUTED_SOURCE_REPLACE_WITH_ZERO,
    MUTED_SOURCE_STILL_CAPTURE,
};


using std::string;
typedef std::function<void(const int id, const uint8_t *, const size_t)> audio_chunk_data_cb;
typedef std::function<void(const int id, const audio_source_capture_status status)> audio_capture_status_change_cb;

#define FRAME_SIZE 2


class AudioCaptureSession {
    OBSSource audio_source;
    OBSSource muting_source;
    source_capture_config muted_handling;

    audio_resampler_t *resampler;
    audio_source_capture_status capture_status;
    bool use_muting_cb_signal = true;
    const int id;
public:
    ThreadsaferCallback<audio_chunk_data_cb> on_caption_cb_handle;
    ThreadsaferCallback<audio_capture_status_change_cb> on_status_cb_handle;

    AudioCaptureSession(
            obs_source_t *audio_source,
            obs_source_t *muting_source,
            audio_chunk_data_cb audio_data_cb,
            audio_capture_status_change_cb status_change_cb,
            resample_info resample_to,
            source_capture_config muted_handling,
            bool send_startup_change_signal,
            int id
    );

    void audio_capture_cb(obs_source_t *source, const struct audio_data *audio, bool muted);

    ~AudioCaptureSession();

    void state_changed_check(bool always_signal = false);

    audio_source_capture_status get_current_capture_status();

    audio_source_capture_status check_source_status();
};
