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

#ifndef OBS_GOOGLE_CAPTION_PLUGIN_DATA_H
#define OBS_GOOGLE_CAPTION_PLUGIN_DATA_H

#define AUDIO_OUTPUT_CAPTURE_STREAMING_SOURCE_NAME "______________________________________audio_output_capture___streaming_source_lets_hope_no_one_ever_names_a_obs_source_list_this"
#define AUDIO_OUTPUT_CAPTURE_RECORDING_SOURCE_NAME "______________________________________audio_output_capture___recording_source_lets_hope_no_one_ever_names_a_obs_source_list_this"

static bool is_audio_output_capture_source_name(const std::string &input) {
    return (
            input == AUDIO_OUTPUT_CAPTURE_STREAMING_SOURCE_NAME
            || input == AUDIO_OUTPUT_CAPTURE_RECORDING_SOURCE_NAME
    );
}

static bool is_streaming_audio_output_capture_source_name(const std::string &input) {
    return input == AUDIO_OUTPUT_CAPTURE_STREAMING_SOURCE_NAME;
}

static bool is_recording_audio_output_capture_source_name(const std::string &input) {
    return input == AUDIO_OUTPUT_CAPTURE_RECORDING_SOURCE_NAME;
}

static const char *streaming_audio_output_capture_source_name() {
    return "All Stream Audio";
}

static const char *recording_audio_output_capture_source_name() {
    return "All Recording Audio";
}

static const std::string corrected_streaming_audio_output_capture_source_name(const std::string &text) {
    if (is_streaming_audio_output_capture_source_name(text))
        return streaming_audio_output_capture_source_name();

    if (is_recording_audio_output_capture_source_name(text))
        return recording_audio_output_capture_source_name();

    return text;
}

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

typedef std::function<void(const int id, const uint8_t *, const size_t)> audio_chunk_data_cb;
typedef std::function<void(const int id, const audio_source_capture_status status)> audio_capture_status_change_cb;

#endif //OBS_GOOGLE_CAPTION_PLUGIN_DATA_H
