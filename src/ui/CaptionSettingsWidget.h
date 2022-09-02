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


#ifndef OBS_GOOGLE_CAPTION_PLUGIN_CAPTIONSETTINGSWIDGET_H
#define OBS_GOOGLE_CAPTION_PLUGIN_CAPTIONSETTINGSWIDGET_H


#include <QWidget>
#include <CaptionStream.h>
#include "ui_CaptionSettingsWidget.h"

#include "../SourceAudioCaptureSession.h"
#include "../log.c"
#include "../SourceCaptioner.h"
#include "../CaptionPluginSettings.h"
#include "OpenCaptionSettingsWidget.h"

class CaptionSettingsWidget : public QWidget, Ui_CaptionSettingsWidget {
Q_OBJECT

    CaptionPluginSettings current_settings;
    string scene_collection_name;
    OpenCaptionSettingsList *caption_settings_widget;

    void accept_current_settings();

private slots:

    void transcript_format_index_change(int index);

    void language_index_change(int index);

    void recording_name_index_change(int index);

    void streaming_name_index_change(int index);

    void virtualcam_name_index_change(int index);

    void sources_combo_index_change(int index);

    void caption_when_index_change(int index);

    void scene_collection_combo_index_change(int index);

    void on_previewPushButton_clicked();

    void on_apiKeyShowPushButton_clicked();

    void on_replacementWordsAddWordPushButton_clicked();

    void on_transcriptFolderPickerPushButton_clicked();

    void on_saveTranscriptsCheckBox_stateChanged(int new_state);

    void on_enabledCheckBox_stateChanged(int new_state);

    void set_show_key(bool set_to_show);

    void apply_ui_scene_collection_settings();

    void update_scene_collection_ui(const string &use_scene_collection_name);

signals:

    void settings_accepted(CaptionPluginSettings new_settings);

    void preview_requested();

public:
    CaptionSettingsWidget(const CaptionPluginSettings &latest_settings);

    void set_settings(const CaptionPluginSettings &new_settings);

    void updateUi();

    void update_sources_visibilities();

private:
    void showEvent(QShowEvent *event) override;
};


#endif //OBS_GOOGLE_CAPTION_PLUGIN_CAPTIONSETTINGSWIDGET_H
