/******************************************************************************
Copyright (C) 2020 by <rat.with.a.compiler@gmail.com>

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

class OutputAudioCaptureSession {
    const int id;
    struct audio_convert_info converter;
    audio_t *audio_output = nullptr;
    const int bytes_per_channel;
    const int track_index;
public:
    ThreadsaferCallback<audio_chunk_data_cb> on_caption_cb_handle;
    ThreadsaferCallback<audio_capture_status_change_cb> on_status_cb_handle;

    OutputAudioCaptureSession(
            int track_index,
            audio_chunk_data_cb audio_data_cb,
            audio_capture_status_change_cb status_change_cb,
            resample_info resample_to,
            int id
    );

    void audio_capture_cb(size_t mix_idx, const struct audio_data *audio);

    ~OutputAudioCaptureSession();
};
