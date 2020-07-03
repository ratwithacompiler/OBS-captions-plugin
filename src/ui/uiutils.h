//
// Created by Rat on 09.10.19.
//

#ifndef OBS_GOOGLE_CAPTION_PLUGIN_UI_UTILS_H
#define OBS_GOOGLE_CAPTION_PLUGIN_UI_UTILS_H

#include "../CaptionPluginManager.h"

static bool captioning_status_string(
        bool enabled,
        bool streaming_output_enabled,
        bool recording_output_enabled,

        const CaptioningState &captioning_state,

        const SourceCaptionerStatus &status,
        const string &source_name,
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

            string source_name_use = string("(") + source_name + ")";
            if (status.audio_capture_status == AUDIO_SOURCE_CAPTURING) {
                string target;
                if (captioning_state.is_captioning_streaming && captioning_state.is_captioning_recording)
                    target = "Stream and Recording";
                else if (captioning_state.is_captioning_streaming)
                    target = "Stream";
                else if (captioning_state.is_captioning_recording)
                    target = "Recording";
                else if (captioning_state.is_captioning_preview)
                    target = "Preview";

                if (!target.empty())
                    target = " for " + target;

                output = "🔴 CC " + source_name_use + target;

            } else if (status.audio_capture_status == AUDIO_SOURCE_MUTED)
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
