//
// Created by Rat on 09.10.19.
//

#ifndef OBS_GOOGLE_CAPTION_PLUGIN_UI_UTILS_H
#define OBS_GOOGLE_CAPTION_PLUGIN_UI_UTILS_H


static bool captioning_status_string(bool enabled, const SourceCaptionerStatus &status, string &output) {
    if (!enabled) {
        output = "Disabled";
    } else {
        if (status.event_type == SOURCE_CAPTIONER_STATUS_EVENT_STOPPED
            || status.event_type == SOURCE_CAPTIONER_STATUS_EVENT_STARTED_ERROR
            || status.event_type == SOURCE_CAPTIONER_STATUS_EVENT_NEW_SETTINGS_STOPPED) {
            output = "Off";
        } else if (status.event_type == SOURCE_CAPTIONER_STATUS_EVENT_STARTED_OK
                   || status.event_type == SOURCE_CAPTIONER_STATUS_EVENT_AUDIO_CAPTURE_STATUS_CHANGE) {

            if (status.audio_capture_status == AUDIO_SOURCE_CAPTURING)
                output = "🔴 Captioning";
            else if (status.audio_capture_status == AUDIO_SOURCE_MUTED)
                output = "Muted";
            else if (status.audio_capture_status == AUDIO_SOURCE_NOT_STREAMED)
                output = "Source not streamed";
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
