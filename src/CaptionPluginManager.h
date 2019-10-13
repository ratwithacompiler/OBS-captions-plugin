//
// Created by Rat on 06.10.19.
//

#ifndef OBS_GOOGLE_CAPTION_PLUGIN_CAPTIONPLUGINMANAGER_H
#define OBS_GOOGLE_CAPTION_PLUGIN_CAPTIONPLUGINMANAGER_H


#include "CaptionPluginSettings.h"
#include "SourceCaptioner.h"

struct CaptioningState {
    bool external_is_streaming = false;
    bool external_is_recording = false;
    bool external_is_preview_open = false;

    bool is_captioning = false;
    bool is_captioning_streaming = false;
    bool is_captioning_recording = false;
    bool is_captioning_preview = false;
};

class CaptionPluginManager : public QObject {
Q_OBJECT

public:
    CaptionPluginSettings plugin_settings;
    SourceCaptioner source_captioner;
    CaptioningState state;

private:

    int update_count = 0;

public:
    CaptionPluginManager(const CaptionPluginSettings &initial_settings);

//    void update_settings(CaptionPluginSettings &new_settings, bool force_update = false);

    void external_state_changed(bool is_live, bool is_preview_open, bool is_recording);

    void update_settings(const CaptionPluginSettings &new_settings);

    bool toggle_enabled();

    void save(obs_data_t *save_data);

    CaptioningState captioning_state();

public slots:

    void set_settings(const CaptionPluginSettings &new_settings);

signals:

    void settings_changed(CaptionPluginSettings new_settings);
};


#endif //OBS_GOOGLE_CAPTION_PLUGIN_CAPTIONPLUGINMANAGER_H
