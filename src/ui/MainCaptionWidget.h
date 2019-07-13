//
// Created by Rat on 2019-06-13.
//

#ifndef OBS_GOOGLE_CAPTION_PLUGIN_MAINWIDGET_H
#define OBS_GOOGLE_CAPTION_PLUGIN_MAINWIDGET_H

#include <QWidget>
#include <src/SourceCaptioner.h>
#include "ui_MainCaptionWidget.h"
#include "CaptionSettingsWidget.h"
#include "../log.c"

#include <concurrentqueue.h>

typedef std::tuple<std::shared_ptr<CaptionResult>, bool, bool, int> ResultTup;

class MainCaptionWidget : public QWidget, Ui_MainCaptionWidget {
Q_OBJECT
    CaptionerSettings current_settings;
    SourceCaptioner captioner;
    CaptionSettingsWidget caption_settings_widget;
    bool is_captioning = false;
    moodycamel::ConcurrentQueue<ResultTup> result_queue;

    std::shared_ptr<CaptionResult> latest_caption_result;
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

    void handle_caption_data_cb(shared_ptr<CaptionResult> caption_result, bool interrupted, bool cleared, int active_delay_sec);

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

    void set_enabled(bool is_enabled);
};


#endif //OBS_GOOGLE_CAPTION_PLUGIN_MAINWIDGET_H
