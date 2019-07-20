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

#include <lib/caption_stream/CaptionStream.h>
#include "log.c"
#include <utils.h>
#include "storage_utils.h"

#define SAVE_ENTRY_NAME "cloud_closed_caption_rat"

static CaptionStreamSettings default_CaptionStreamSettings() {
    uint download_start_delay_ms = 4000;
    download_start_delay_ms = 40;
    return {
            5000,
            5000,
            180'000,
            50,
            download_start_delay_ms
    };
};


static ContinuousCaptionStreamSettings default_ContinuousCaptionStreamSettings() {
    return {
            260,
            275,
            10,
            default_CaptionStreamSettings()
    };
};

static CaptionFormatSettings default_CaptionFormatSettings() {
    return {
            32,
            3,
            false,
            {"niger", "nigger", "fag", "faggot", "chink"}, // default banned words
            true,
            25.0,
    };
};


static CaptionSourceSettings default_CaptionSourceSettings() {
    return {
            "",
            CAPTION_SOURCE_MUTE_TYPE_FROM_OWN_SOURCE,
            ""
    };
}

static CaptionerSettings default_CaptionerSettings() {
    return CaptionerSettings(
            false,
            default_CaptionSourceSettings(),
            default_CaptionFormatSettings(),
            default_ContinuousCaptionStreamSettings()
    );
};


static void enforce_sensible_values(CaptionerSettings &settings) {
    if (settings.format_settings.caption_line_count <= 0 || settings.format_settings.caption_line_count > 4)
        settings.format_settings.caption_line_count = 1;
}

static CaptionerSettings load_obs_CaptionerSettings(obs_data_t *load_data) {
    info_log("obs load event");

    obs_data_t *obj = obs_data_get_obj(load_data, SAVE_ENTRY_NAME);
    auto settings = default_CaptionerSettings();

    printf("\n\n");
    settings.print();
    if (!obj) {
        info_log("first time loading, keeping default settings");
    } else {
        obs_data_set_default_bool(obj, "enabled", settings.enabled);
        obs_data_set_default_bool(obj, "caption_insert_newlines", settings.format_settings.caption_insert_newlines);
        obs_data_set_default_int(obj, "caption_line_count", settings.format_settings.caption_line_count);
        obs_data_set_default_string(obj, "manual_banned_words", "");
        obs_data_set_default_string(obj, "source_name", settings.caption_source_settings.caption_source_name.c_str());
        obs_data_set_default_string(obj, "mute_source_name", "");
        obs_data_set_default_string(obj, "source_caption_when", "");
        obs_data_set_default_double(obj, "caption_timeout_secs", settings.format_settings.caption_timeout_seconds);
        obs_data_set_default_bool(obj, "caption_timeout_enabled", settings.format_settings.caption_timeout_enabled);


        settings.enabled = obs_data_get_bool(obj, "enabled");
        settings.format_settings.caption_insert_newlines = obs_data_get_bool(obj, "caption_insert_newlines");
        settings.format_settings.caption_line_count = (int) obs_data_get_int(obj, "caption_line_count");

        settings.caption_source_settings.caption_source_name = obs_data_get_string(obj, "source_name");
        settings.caption_source_settings.mute_source_name = obs_data_get_string(obj, "mute_source_name");

        settings.format_settings.caption_timeout_enabled = obs_data_get_bool(obj, "caption_timeout_enabled");
        settings.format_settings.caption_timeout_seconds = obs_data_get_double(obj, "caption_timeout_secs");

        string mute_when_str = obs_data_get_string(obj, "source_caption_when");
        settings.caption_source_settings.mute_when = string_to_mute_setting(mute_when_str, CAPTION_SOURCE_MUTE_TYPE_FROM_OWN_SOURCE);

        string banned_words_line = obs_data_get_string(obj, "manual_banned_words");
        settings.format_settings.manual_banned_words = string_to_banned_words(banned_words_line);

        enforce_sensible_values(settings);
    }
    obs_data_release(obj);
    printf("\n\n");
    settings.print();
    return settings;

}

static void save_obs_CaptionerSettings(obs_data_t *save_data, CaptionerSettings &settings) {
    info_log("obs save event");
    obs_data_t *obj = obs_data_create();

    obs_data_set_bool(obj, "enabled", settings.enabled);
    obs_data_set_int(obj, "caption_line_count", settings.format_settings.caption_line_count);
    obs_data_set_bool(obj, "caption_insert_newlines", settings.format_settings.caption_insert_newlines);
//    obs_data_set_bool(obj, "caption_insert_newlines", settings.format_settings.caption_insert_newlines);


    string caption_when = mute_setting_to_string(settings.caption_source_settings.mute_when, "");
    obs_data_set_string(obj, "source_caption_when", caption_when.c_str());

    obs_data_set_string(obj, "source_name", settings.caption_source_settings.caption_source_name.c_str());
    obs_data_set_string(obj, "mute_source_name", settings.caption_source_settings.mute_source_name.c_str());

    obs_data_set_bool(obj, "caption_timeout_enabled", settings.format_settings.caption_timeout_enabled);
    obs_data_set_double(obj, "caption_timeout_secs", settings.format_settings.caption_timeout_seconds);

    string banned_words_line;
    if (!settings.format_settings.manual_banned_words.empty()) {
//        info_log("savingggggg %d", settings.format_settings.manual_banned_words.size());
        words_to_string(settings.format_settings.manual_banned_words, banned_words_line);
    }
    obs_data_set_string(obj, "manual_banned_words", banned_words_line.c_str());

    obs_data_set_obj(save_data, SAVE_ENTRY_NAME, obj);
    obs_data_release(obj);
}

static bool is_stream_live() {
    obs_output_t *output = obs_frontend_get_streaming_output();
    bool live = !!output;
    obs_output_release(output);
    return live;
}
