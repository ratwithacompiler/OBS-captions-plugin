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

#include <concurrentqueue.h>

typedef std::tuple<std::shared_ptr<OutputCaptionResult>, bool, bool, string> ResultTup;

class MainCaptionWidget : public QWidget, Ui_MainCaptionWidget {
Q_OBJECT
    CaptionerSettings current_settings;
    SourceCaptioner captioner;
    CaptionSettingsWidget caption_settings_widget;
    bool is_captioning = false;
    moodycamel::ConcurrentQueue<ResultTup> result_queue;

    std::shared_ptr<OutputCaptionResult> latest_caption_result;
    string latest_caption_text_history;
    bool cleared = false;

signals:

    void process_item_queue();

    void enabled_state_changed(bool is_enabled);

private:

    void showEvent(QShowEvent *event) override;

    void hideEvent(QHideEvent *event) override;

    void apply_changed_settings(CaptionerSettings new_settings, bool force_update = false);

    void apply_changed_settings(CaptionerSettings new_settings, bool is_live, bool is_preview_open,
                                bool force_update = false);

    void do_process_item_queue();

private slots:

    void handle_caption_data_cb(
            shared_ptr<OutputCaptionResult> caption_result,
            bool interrupted,
            bool cleared,
            string recent_caption_text
    );

    void handle_audio_capture_status_change(const int new_status);

    void accept_widget_settings(CaptionerSettings new_settings);

public slots:

    void enabled_state_checkbox_changed(int new_checkbox_state);

    void show_settings();

    void set_settings(CaptionerSettings new_settings);

public:
    explicit MainCaptionWidget(CaptionerSettings initial_settings);

    void menu_button_clicked();

    void save_event_cb(obs_data_t *save_data);

    virtual ~MainCaptionWidget();

    void handle_caption_result_tup(ResultTup &tup);

    void update_caption_text_ui();

    void external_state_changed();

    void stream_started_event();
    void stream_stopped_event();

    void set_enabled(bool is_enabled);
};


#endif //OBS_GOOGLE_CAPTION_PLUGIN_MAINWIDGET_H
