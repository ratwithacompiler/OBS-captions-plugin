//
// Created by Rat on 09.10.19.
//

#ifndef OBS_GOOGLE_CAPTION_PLUGIN_UI_UTILS_H
#define OBS_GOOGLE_CAPTION_PLUGIN_UI_UTILS_H

#include "../CaptionPluginManager.h"
#include <QComboBox>
#include "../log.c"

typedef std::tuple<string, string, uint32_t> ObsSourceTup;

static TextOutputSettings default_TextOutputSettings() {
    return {
            true,
            "",
            60,
            4,
            false,
            CAPITALIZATION_NORMAL,
//            true
    };
}


static vector<ObsSourceTup> get_obs_sources() {
    vector<ObsSourceTup> sources;

    auto cb = [](void *param, obs_source_t *source) {
        auto sources = reinterpret_cast<vector<ObsSourceTup> *>(param);
        if (!sources) {
            return false;
        }

        const char *source_type = obs_source_get_id(source);
        const char *name = obs_source_get_name(source);

        if (!name || !source_type)
            return true;

        uint32_t caps = obs_source_get_output_flags(source);
        sources->push_back(ObsSourceTup(string(name), string(source_type), caps));

//        info_log("source: %s id: %s", name, id);
        return true;
    };
    obs_enum_sources(cb, &sources);
    return sources;
}


static vector<string> get_audio_sources() {
    auto sources = get_obs_sources();
    vector<string> audio_sources;

    for (auto &a_source: sources) {
        if (std::get<2>(a_source) & OBS_SOURCE_AUDIO) {
            audio_sources.push_back(std::get<0>(a_source));
        }
    }

    return audio_sources;
}


static vector<string> get_text_sources() {
    auto sources = get_obs_sources();
    vector<string> text_sources;

    for (auto &a_source: sources) {
        string &source_type = std::get<1>(a_source);
        if (source_type == "text_gdiplus" || source_type == "text_gdiplus_v2"
            || source_type == "text_ft2_source" || source_type == "text_ft2_source_v2"
            || source_type == "text_pango_source") {
            text_sources.push_back(std::get<0>(a_source));
        }
    }

    return text_sources;
}

static void setup_combobox_texts(QComboBox &comboBox,
                                 const vector<string> &items
) {
    while (comboBox.count())
        comboBox.removeItem(0);

    for (auto &a_item: items) {
        comboBox.addItem(QString::fromStdString(a_item));
    }
}

static int combobox_set_data_int(QComboBox &combo_box, const int data, int default_index) {
    int index = combo_box.findData(data);
    if (index == -1)
        index = default_index;

    combo_box.setCurrentIndex(index);
    return index;
}

static void setup_combobox_capitalization(QComboBox &comboBox) {
    while (comboBox.count())
        comboBox.removeItem(0);

    comboBox.addItem("Normal english like.", 0);
    comboBox.addItem("ALL. CAPS. ", 1);
    comboBox.addItem("all. lowercase.", 2);
}

static string transcript_format_extension(const string &format, const string &fallback) {
    if (format == "raw")
        return "log";

    if (format == "txt" || format == "srt")
        return format;

    if (format == "txt_plain")
        return "txt";

    return fallback;
}

static bool captioning_status_string(
        bool enabled,
        bool streaming_output_enabled,
        bool recording_output_enabled,

        const CaptioningState &captioning_state,

        const SourceCaptionerStatus &status,
        string &output
) {
    debug_log("status %d", status.event_type);
    if (!enabled) {
        output = "CC Disabled";
    } else {
        if (status.event_type == SOURCE_CAPTIONER_STATUS_EVENT_STOPPED
            || status.event_type == SOURCE_CAPTIONER_STATUS_EVENT_NEW_SETTINGS_STOPPED) {
            if (streaming_output_enabled && recording_output_enabled)
                output = "Not streaming/recording. CC Off";
            else if (streaming_output_enabled)
                output = "Not streaming. CC Off";
            else if (recording_output_enabled)
                output = "Not recording. CC Off";
            else
                output = "Offline";

        } else if (status.event_type == SOURCE_CAPTIONER_STATUS_EVENT_STARTED_ERROR) {
            output = "Off";
        } else if (status.event_type == SOURCE_CAPTIONER_STATUS_EVENT_STARTED_OK
                   || status.event_type == SOURCE_CAPTIONER_STATUS_EVENT_AUDIO_CAPTURE_STATUS_CHANGE) {

            const string source_name = corrected_streaming_audio_output_capture_source_name(
                    status.settings.scene_collection_settings.caption_source_settings.caption_source_name);
            const string mute_source_name = status.settings.scene_collection_settings.caption_source_settings.active_mute_source_name();

            const string source_name_use = "(" + source_name + ")";
            const string mute_source_name_use = "(" + mute_source_name + ")";

            if (status.audio_capture_status == AUDIO_SOURCE_CAPTURING) {
                string target;
                if (captioning_state.is_captioning_streaming && captioning_state.is_captioning_recording &&
                    captioning_state.is_captioning_virtualcam)
                    target = "Stream & Recording & Virt Cam";
                else if (captioning_state.is_captioning_streaming && captioning_state.is_captioning_recording)
                    target = "Stream & Recording";
                else if (captioning_state.is_captioning_streaming)
                    target = "Stream";
                else if (captioning_state.is_captioning_recording)
                    target = "Recording";
                else if (captioning_state.is_captioning_virtualcam)
                    target = "Virtual Cam";
                else if (captioning_state.is_captioning_preview)
                    target = "Preview";
                else if (captioning_state.is_captioning_text_output)
                    target = "Text Source";

                if (!target.empty())
                    target = " for " + target;

                output = "🔴 CC " + source_name_use + target;

            } else if (status.audio_capture_status == AUDIO_SOURCE_MUTED)
                output = "Muted " + mute_source_name_use;
            else if (status.audio_capture_status == AUDIO_SOURCE_NOT_STREAMED)
                output = "Source not streamed " + mute_source_name_use;
            else {
                output = "??";
                return false;
            }
        } else {
            output = "?";
            return false;
        }
    }
    return true;
}

#endif //OBS_GOOGLE_CAPTION_PLUGIN_UI_UTILS_H
