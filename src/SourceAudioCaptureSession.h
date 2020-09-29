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
#include "data.h"

class SourceAudioCaptureSession {
    OBSSource audio_source;
    OBSSource muting_source;
    source_capture_config muted_handling;

    audio_resampler_t *resampler;
    audio_source_capture_status capture_status;
    bool use_muting_cb_signal = true;
    const int id;
    const int bytes_per_channel;
public:
    ThreadsaferCallback<audio_chunk_data_cb> on_caption_cb_handle;
    ThreadsaferCallback<audio_capture_status_change_cb> on_status_cb_handle;

    SourceAudioCaptureSession(
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

    ~SourceAudioCaptureSession();

    void state_changed_check(bool always_signal = false);

    audio_source_capture_status get_current_capture_status();

    audio_source_capture_status check_source_status();
};
