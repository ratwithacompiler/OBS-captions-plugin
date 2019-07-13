
#ifndef OBS_GOOGLE_CAPTION_PLUGIN_CAPTIONSETTINGSWIDGET_H
#define OBS_GOOGLE_CAPTION_PLUGIN_CAPTIONSETTINGSWIDGET_H


#include <QWidget>
#include <lib/caption_stream/CaptionStream.h>
#include "ui_CaptionSettingsWidget.h"
#include "../AudioCaptureSession.h"
#include "../log.c"
#include "../SourceCaptioner.h"

class CaptionSettingsWidget : public QWidget, Ui_CaptionSettingsWidget {
Q_OBJECT

    CaptionerSettings latest_settings;

    void accept_current_settings();


private slots:

    void caption_when_index_change(int index);


signals:

    void settings_accepted(CaptionerSettings new_settings);


public:
    CaptionSettingsWidget(CaptionerSettings latest_settings);

    void set_settings(CaptionerSettings new_settings);

    void updateUi();

    void update_other_source_visibility(CaptionSourceMuteType mute_state);
};


#endif //OBS_GOOGLE_CAPTION_PLUGIN_CAPTIONSETTINGSWIDGET_H
