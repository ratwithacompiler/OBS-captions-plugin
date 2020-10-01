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

#define AUDIO_OUTPUT_CAPTURE_ALL_AUDIO_SOURCE_NAME "______________________________________audio_output_capture___all_audio_source_lets_hope_no_one_ever_names_a_obs_source_list_this_"

#include <string>

static bool is_all_audio_output_capture_source_data(const std::string &data) {
    return data.find(AUDIO_OUTPUT_CAPTURE_ALL_AUDIO_SOURCE_NAME) == 0;
}

static int all_audio_output_capture_source_track_index(const std::string &data) {
    if (!is_all_audio_output_capture_source_data(data))
        return -1;

    const std::string rest = data.substr(strlen(AUDIO_OUTPUT_CAPTURE_ALL_AUDIO_SOURCE_NAME));
    try {
        return std::stoi(rest, nullptr);
    }
    catch (...) {
        return -1;
    }
}

static const std::string all_audio_output_capture_source_name(const int track_index) {
    return "Stream Audio (Track " + std::to_string(track_index + 1) + ")";
}

static const std::string all_audio_output_capture_source_data(const int track_index) {
    return AUDIO_OUTPUT_CAPTURE_ALL_AUDIO_SOURCE_NAME + std::to_string(track_index);
}


static const std::string corrected_streaming_audio_output_capture_source_name(const std::string &data) {
    const int track_index = all_audio_output_capture_source_track_index(data);
    if (track_index >= 0) {
        return all_audio_output_capture_source_name(track_index);
    }

    return data;
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
