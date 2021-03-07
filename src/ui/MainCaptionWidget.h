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


#ifndef OBS_GOOGLE_CAPTION_PLUGIN_MAINWIDGET_H
#define OBS_GOOGLE_CAPTION_PLUGIN_MAINWIDGET_H

#include <QWidget>
#include <src/SourceCaptioner.h>
#include "ui_MainCaptionWidget.h"
#include "CaptionSettingsWidget.h"
#include "../log.c"

#include <cameron314/blockingconcurrentqueue.h>
#include "../CaptionPluginSettings.h"
#include "../CaptionPluginManager.h"

typedef std::tuple<std::shared_ptr<OutputCaptionResult>, bool, string> ResultTup;


class MainCaptionWidget : public QWidget, Ui_MainCaptionWidget {
Q_OBJECT
    CaptionPluginManager &plugin_manager;
    CaptionSettingsWidget caption_settings_widget;

    std::unique_ptr<ResultTup> latest_caption_result_tup;
signals:

    void process_item_queue();

private:

    void showEvent(QShowEvent *event) override;

    void hideEvent(QHideEvent *event) override;

    void do_process_item_queue();

    void accept_widget_settings(CaptionPluginSettings new_settings);

    void handle_caption_data_cb(
            shared_ptr<OutputCaptionResult> caption_result,
            bool cleared,
            string recent_caption_text
    );

    void handle_source_capture_status_change(shared_ptr<SourceCaptionerStatus> status);

    void settings_changed_event(CaptionPluginSettings new_settings);

public slots:

    void enabled_state_checkbox_changed(int new_checkbox_state);

    void show_settings_dialog();


public:
    MainCaptionWidget(CaptionPluginManager &plugin_manager);

    void show_self();

    virtual ~MainCaptionWidget();

    void handle_caption_result_tup(ResultTup &tup);

    void update_caption_text_ui();

    void external_state_changed();

    void scene_collection_changed();

    void stream_started_event();

    void stream_stopped_event();

    void recording_started_event();

    void recording_stopped_event();

    void virtualcam_started_event();

    void virtualcam_stopped_event();
};


#endif //OBS_GOOGLE_CAPTION_PLUGIN_MAINWIDGET_H
