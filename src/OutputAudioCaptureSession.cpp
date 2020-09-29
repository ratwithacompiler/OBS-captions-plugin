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

#include <utility>
#include <util/platform.h>

#include "OutputAudioCaptureSession.h"
#include "log.c"

static void audio_captured(void *param, size_t mix_idx, struct audio_data *audio) {
    auto session = reinterpret_cast<OutputAudioCaptureSession *>(param);
    if (session)
        session->audio_capture_cb(mix_idx, audio);
}

OutputAudioCaptureSession::OutputAudioCaptureSession(
        bool use_streaming_output,
        audio_chunk_data_cb audio_data_cb,
        audio_capture_status_change_cb status_change_cb,
        resample_info resample_to,
        int id
) :
        on_caption_cb_handle(audio_data_cb),
        on_status_cb_handle(status_change_cb),
        bytes_per_channel(get_audio_bytes_per_channel(resample_to.format)),
        id(id) {
    debug_log("OutputAudioCaptureSession()");

    obs_audio_info backend_audio_settings;
    if (!obs_get_audio_info(&backend_audio_settings))
        throw std::string("Failed to get OBS audio info");
    debug_log("output audio_info %d: %d, %d",
              use_streaming_output, backend_audio_settings.samples_per_sec, backend_audio_settings.speakers);

    if (!bytes_per_channel)
        throw std::string("Failed to get frame bytes size per channel");

    converter.speakers = resample_to.speakers;
    converter.format = resample_to.format;
    converter.samples_per_sec = resample_to.samples_per_sec;

    obs_output_t *output = use_streaming_output ? obs_frontend_get_streaming_output() : obs_frontend_get_recording_output();
//    printf("has output %d\n", !!output);
    audio_output = obs_output_audio(output);
    if (!audio_output) {
        throw std::string("couldn't get output audio");
    }

//    printf("setting up audio cb\n");
    audio_output_connect(audio_output, 0, &converter, audio_captured, this);
}

void OutputAudioCaptureSession::audio_capture_cb(size_t mix_idx, const struct audio_data *audio) {
    if (!on_caption_cb_handle.callback_fn)
        return;

    if (!audio || !audio->frames)
        return;

    unsigned int size = audio->frames * bytes_per_channel;
    {
        std::lock_guard<std::recursive_mutex> lock(on_caption_cb_handle.mutex);
        if (on_caption_cb_handle.callback_fn)
            on_caption_cb_handle.callback_fn(id, audio->data[0], size);
    }
}

OutputAudioCaptureSession::~OutputAudioCaptureSession() {
    on_caption_cb_handle.clear();
    on_status_cb_handle.clear();

    audio_output_disconnect(audio_output, 0, audio_captured, this);
    debug_log("OutputAudioCaptureSessionession() deaded");
}
