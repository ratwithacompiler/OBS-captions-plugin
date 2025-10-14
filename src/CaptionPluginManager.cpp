//
// Created by Rat on 06.10.19.
//

#include "CaptionPluginManager.h"
#include "caption_stream_helper.cpp"

CaptionPluginManager::CaptionPluginManager(const CaptionPluginSettings &initial_settings) :
        plugin_settings(initial_settings),
        source_captioner(initial_settings.enabled, initial_settings.source_cap_settings, current_scene_collection_name(), false) {
}

void CaptionPluginManager::external_state_changed(
        bool is_live,
        bool is_preview_open,
        bool is_recording,
        bool is_virtualcam_on,
        const string &scene_collection_name) {
    state.external_is_streaming = is_live;
    state.external_is_recording = is_recording;
    state.external_is_preview_open = is_preview_open;
    state.external_is_virtualcam_on = is_virtualcam_on;
    state.external_scene_collection_name = scene_collection_name;

    update_settings(plugin_settings);
}

void CaptionPluginManager::set_settings(const CaptionPluginSettings &new_settings) {
    update_settings(new_settings);
}

void CaptionPluginManager::update_settings(const CaptionPluginSettings &new_settings) {
    // apply settings if they are different
    const SourceCaptionerSettings &source_settings = new_settings.source_cap_settings;
    string scene_collection_name_relevant = state.external_scene_collection_name;
    auto scene_col_settings = source_settings.get_scene_collection_settings(scene_collection_name_relevant);

    const bool transcripts_set_up = source_settings.transcript_settings.hasBaseSettings(); // checks transcripts enabled
    const bool streaming_transcripts_enabled = transcripts_set_up && source_settings.transcript_settings.streaming_transcripts_enabled;
    const bool recording_transcripts_enabled = transcripts_set_up && source_settings.transcript_settings.recording_transcripts_enabled;

    const bool is_streaming_relevant =
            state.external_is_streaming && (source_settings.streaming_output_enabled || streaming_transcripts_enabled);
    const bool is_recording_relevant =
            state.external_is_recording && (source_settings.recording_output_enabled || recording_transcripts_enabled);

    const bool virtualcam_transcripts_enabled = transcripts_set_up && source_settings.transcript_settings.virtualcam_transcripts_enabled;
    const bool is_virtualcam_relevant = state.external_is_virtualcam_on && virtualcam_transcripts_enabled;

    const bool is_preview_relevant = state.external_is_preview_open;

    const bool any_output_active = state.external_is_streaming || state.external_is_recording || state.external_is_virtualcam_on;

    bool is_text_output_relevant = false;
    if (any_output_active) {
        for (const auto &i: scene_col_settings.text_outputs) {
            if (i.isValidEnabled()) {
                is_text_output_relevant = true;
                break;
            }
        }
    }

    bool is_file_output_relevant = false;
    if (any_output_active && source_settings.file_output_settings.isValidEnabled()) {
        is_file_output_relevant = true;
    }

    const bool equal_settings = new_settings == plugin_settings;
    const bool do_captioning = (
            new_settings.enabled &&
            (is_streaming_relevant || is_recording_relevant || is_preview_relevant || is_text_output_relevant || is_file_output_relevant || is_virtualcam_relevant));

    info_log("enabled: %d, "

             "is_streaming %d, "
             "streaming_output_enabled %d, "
             "streaming_transcripts_enabled %d, "
             "is_streaming_relevant: %d, "

             "is_recording %d, "
             "recording_output_enabled %d, "
             "recording_transcripts_enabled %d, "
             "is_recording_relevant: %d, "

             "is_virtualcam_on %d, "
             "virtualcam_transcripts_enabled %d, "
             "is_virtualcam_relevant %d, "

             "is_preview_open %d, "
             "is_text_output_relevant %d, "
             "is_file_output_relevant %d, "

             "scene_collection_name: %s, "
             "source:  '%s', "

             "equal_settings %d, "
             "do_captioning %d",

             new_settings.enabled,

             state.external_is_streaming,
             source_settings.streaming_output_enabled,
             streaming_transcripts_enabled,
             is_streaming_relevant,

             state.external_is_recording,
             source_settings.recording_output_enabled,
             recording_transcripts_enabled,
             is_recording_relevant,

             state.external_is_virtualcam_on,
             virtualcam_transcripts_enabled,
             is_virtualcam_relevant,

             state.external_is_preview_open,
             is_text_output_relevant,
             is_file_output_relevant,

             scene_collection_name_relevant.c_str(),
             scene_col_settings.caption_source_settings.caption_source_name.c_str(),

             equal_settings,
             do_captioning
    );


    if (update_count != 0
        && equal_settings
        && do_captioning == state.is_captioning
        && is_streaming_relevant == state.is_captioning_streaming
        && is_recording_relevant == state.is_captioning_recording
        && is_virtualcam_relevant == state.is_captioning_virtualcam
        && is_preview_relevant == state.is_captioning_preview
        && is_text_output_relevant == state.is_captioning_text_output
        && is_file_output_relevant == state.is_captioning_file_output
        && scene_collection_name_relevant == state.captioning_scene_collection_name
            ) {
        info_log("settings unchanged, ignoring");
        return;
    }
    update_count++;
    plugin_settings = new_settings;

    state.is_captioning = do_captioning;
    state.is_captioning_streaming = is_streaming_relevant;
    state.is_captioning_recording = is_recording_relevant;
    state.is_captioning_virtualcam = is_virtualcam_relevant;
    state.is_captioning_preview = is_preview_relevant;
    state.is_captioning_text_output = is_text_output_relevant;
    state.is_captioning_file_output = is_file_output_relevant;
    state.captioning_scene_collection_name = scene_collection_name_relevant;

    source_captioner.set_enabled(new_settings.enabled);
    if (do_captioning) {
        info_log("caption settings changed, starting captioning");
        bool worked = source_captioner.start_caption_stream(source_settings, scene_collection_name_relevant);
        if (worked)
            info_log("captioning start ok");
        else
            info_log("captioning start failed");

    } else {
        info_log("settings changed, disabling captioning");
        source_captioner.set_settings(source_settings, scene_collection_name_relevant);
    }

//    debug_log("emit settings_changed");
    emit settings_changed(new_settings);
}

bool CaptionPluginManager::toggle_enabled() {
    CaptionPluginSettings new_settings = plugin_settings;
    new_settings.enabled = !new_settings.enabled;
    update_settings(new_settings);

    debug_log("toggled enabled,  %d -> %d", !new_settings.enabled, new_settings.enabled);
    return new_settings.enabled;
}

CaptioningState CaptionPluginManager::captioning_state() {
    return state;
}
