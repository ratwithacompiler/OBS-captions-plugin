//
// Created by Rat on 09.10.19.
//

#ifndef OBS_GOOGLE_CAPTION_PLUGIN_UI_UTILS_H
#define OBS_GOOGLE_CAPTION_PLUGIN_UI_UTILS_H


static bool captioning_status_string(
        bool enabled,
        const SourceCaptionerStatus &status,
        const string &source_name,
        string &output
) {
    if (!enabled) {
        output = "Disabled";
    } else {
        if (status.event_type == SOURCE_CAPTIONER_STATUS_EVENT_STOPPED
            || status.event_type == SOURCE_CAPTIONER_STATUS_EVENT_NEW_SETTINGS_STOPPED) {
            output = "Offline";
        } else if (status.event_type == SOURCE_CAPTIONER_STATUS_EVENT_STARTED_ERROR) {
            output = "Off";
        } else if (status.event_type == SOURCE_CAPTIONER_STATUS_EVENT_STARTED_OK
                   || status.event_type == SOURCE_CAPTIONER_STATUS_EVENT_AUDIO_CAPTURE_STATUS_CHANGE) {

            string source_name_use = string("[") + source_name + "]";;

            if (status.audio_capture_status == AUDIO_SOURCE_CAPTURING)
                output = "🔴 Captioning " + source_name_use;
            else if (status.audio_capture_status == AUDIO_SOURCE_MUTED)
                output = "Muted " + source_name_use;
            else if (status.audio_capture_status == AUDIO_SOURCE_NOT_STREAMED)
                output = "Source not streamed " + source_name_use;
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
