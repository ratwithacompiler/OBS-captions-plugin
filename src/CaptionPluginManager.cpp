//
// Created by Rat on 06.10.19.
//

#include "CaptionPluginManager.h"
#include "caption_stream_helper.cpp"

CaptionPluginManager::CaptionPluginManager(const CaptionPluginSettings &initial_settings) :
        plugin_settings(initial_settings),
        source_captioner(initial_settings.source_cap_settings, current_scene_collection_name(), false) {
}

void CaptionPluginManager::external_state_changed(
        bool is_live,
        bool is_preview_open,
        bool is_recording,
        const string &scene_collection_name) {
    state.external_is_streaming = is_live;
    state.external_is_recording = is_recording;
    state.external_is_preview_open = is_preview_open;
    state.external_scene_collection_name = scene_collection_name;

    update_settings(plugin_settings);
}

void CaptionPluginManager::set_settings(const CaptionPluginSettings &new_settings) {
    update_settings(new_settings);
}

void CaptionPluginManager::update_settings(const CaptionPluginSettings &new_settings) {
    // apply settings if they are different
    const SourceCaptionerSettings &source_settings = new_settings.source_cap_settings;
    bool is_streaming_relevant = state.external_is_streaming && source_settings.streaming_output_enabled;
    bool is_recording_relevant = state.external_is_recording && source_settings.recording_output_enabled;
    bool is_preview_relevant = state.external_is_preview_open;
    string scene_collection_name_relevant = state.external_scene_collection_name;

    auto cap_source_settings = source_settings.get_caption_source_settings_ptr(scene_collection_name_relevant);
    info_log("enabled: %d, "

             "is_live %d, "
             "is_streaming_relevant: %d, "

             "is_recording %d, "
             "is_recording_relevant: %d, "

             "is_preview_open %d, "
             "scene_collection_name: %s, "
             "source:  %s",

             new_settings.enabled,

             state.external_is_streaming,
             is_streaming_relevant,

             state.external_is_recording,
             is_recording_relevant,

             state.external_is_preview_open,
             scene_collection_name_relevant.c_str(),
             cap_source_settings ? cap_source_settings->caption_source_name.c_str() : "nope ?????");

    bool equal_settings = new_settings == plugin_settings;
    bool do_captioning = (is_streaming_relevant || is_recording_relevant || is_preview_relevant) && new_settings.enabled;

    if (update_count != 0
        && equal_settings
        && do_captioning == state.is_captioning
        && is_streaming_relevant == state.is_captioning_streaming
        && is_recording_relevant == state.is_captioning_recording
        && is_preview_relevant == state.is_captioning_preview
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
    state.is_captioning_preview = is_preview_relevant;
    state.captioning_scene_collection_name = scene_collection_name_relevant;

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
