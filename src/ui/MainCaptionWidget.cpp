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


#include "MainCaptionWidget.h"
#include <QTimer>
#include <QThread>
#include "../caption_stream_helper.cpp"
#include "../log.c"

MainCaptionWidget::MainCaptionWidget(CaptionerSettings initial_settings) :
        QWidget(),
        Ui_MainCaptionWidget(),
        current_settings(initial_settings),
        captioner(initial_settings),
        caption_settings_widget(initial_settings) {
    info_log("MainCaptionWidget");
    enforce_sensible_values(current_settings);
    setupUi(this);

    this->captionHistoryPlainTextEdit->setPlainText("");
    this->captionLinesPlainTextEdit->setPlainText("");

    QObject::connect(this->enabledCheckbox, &QCheckBox::stateChanged, this, &MainCaptionWidget::enabled_state_checkbox_changed);
    QObject::connect(this->settingsToolButton, &QToolButton::clicked, this, &MainCaptionWidget::show_settings);
    statusTextLabel->hide();

    QObject::connect(this, &MainCaptionWidget::process_item_queue, this, &MainCaptionWidget::do_process_item_queue);

    QObject::connect(&caption_settings_widget, &CaptionSettingsWidget::settings_accepted,
                     this, &MainCaptionWidget::accept_widget_settings);

    QObject::connect(&captioner, &SourceCaptioner::caption_result_received,
                     this, &MainCaptionWidget::handle_caption_data_cb, Qt::QueuedConnection);

    QObject::connect(&captioner, &SourceCaptioner::audio_capture_status_changed,
                     this, &MainCaptionWidget::handle_audio_capture_status_change, Qt::QueuedConnection);

    apply_changed_settings(initial_settings, true);
}

void MainCaptionWidget::apply_changed_settings(CaptionerSettings new_settings, bool force_update) {
    apply_changed_settings(new_settings, is_stream_live(), isVisible(), force_update);
}

void MainCaptionWidget::apply_changed_settings(CaptionerSettings new_settings,
                                               bool is_live,
                                               bool is_preview_open,
                                               bool force_update) {
    // apply settings if they are different
    printf("is_stream_live() %d, isVisible() %d source: '%s'\n", is_stream_live(), isVisible(),
           new_settings.caption_source_settings.caption_source_name.c_str());
    printf("is_live %d, is_preview_open %d \n", is_live, is_preview_open);
    printf("is_live %d, is_preview_open %d \n", is_live, is_preview_open);
    enforce_sensible_values(new_settings);
    bool equal_settings = current_settings == new_settings;
    bool do_captioning = (is_live || is_preview_open) && new_settings.enabled;

    if (current_settings.enabled != new_settings.enabled)
            emit this->enabled_state_changed(new_settings.enabled);

    if (!force_update && (equal_settings && do_captioning == this->is_captioning)) {
        info_log("settings unchagned, ignoring");
        return;
    }

    current_settings = new_settings;
    this->is_captioning = do_captioning;

    if (current_settings.enabled != enabledCheckbox->isChecked()) {
        info_log("updating enabled checkbox");
        const QSignalBlocker blocker(enabledCheckbox);
        enabledCheckbox->setChecked(current_settings.enabled);
    }

    latest_caption_result = nullptr;
    update_caption_text_ui();

    if (do_captioning) {
        info_log("settings chagned, starting captioning");
        bool worked = captioner.set_settings(new_settings);
        if (!worked) {
            this->handle_audio_capture_status_change(-1);
        }

    } else {
        info_log("settings chagned, disabling captioning");
        captioner.clear_settings();
        this->handle_audio_capture_status_change(-1);
    }
}

void MainCaptionWidget::showEvent(QShowEvent *event) {
    info_log("show event");
    QWidget::showEvent(event);
    update_caption_text_ui();
    external_state_changed();
}

void MainCaptionWidget::hideEvent(QHideEvent *event) {
    QWidget::hideEvent(event);

    info_log("hide event");
    update_caption_text_ui();
    external_state_changed();
}

void MainCaptionWidget::do_process_item_queue() {
    if (QThread::currentThread() != this->thread()) {
        return;
    }

    ResultTup tup;
    int cnt = 0;
    while (result_queue.try_dequeue(tup)) {
        cnt++;
    }

    if (cnt) {
        handle_caption_result_tup(tup);
        if (isVisible())
            this->update_caption_text_ui();
    }
//    info_log("handle_caption_data");
}

void MainCaptionWidget::update_caption_text_ui() {
    if (!latest_caption_result)
        return;

    this->captionHistoryPlainTextEdit->setPlainText(latest_caption_text_history.c_str());
    QTextCursor cursor1 = this->captionHistoryPlainTextEdit->textCursor();
    cursor1.atEnd();
    this->captionHistoryPlainTextEdit->setTextCursor(cursor1);
    this->captionHistoryPlainTextEdit->ensureCursorVisible();

    if (!this->cleared) {
        string single_caption_line;
        for (string &a_line: latest_caption_result->output_lines) {
//                info_log("a line: %s", a_line.c_str());
            if (!single_caption_line.empty())
                single_caption_line.push_back('\n');

            single_caption_line.append(a_line);
        }

        this->captionLinesPlainTextEdit->setPlainText(single_caption_line.c_str());
    }
}


void MainCaptionWidget::handle_caption_result_tup(ResultTup &tup) {
    auto caption_result = std::get<0>(tup);
//    bool interrupted = std::get<1>(tup);
    bool was_cleared = std::get<2>(tup);
    bool active_delay_sec = std::get<3>(tup);
    string recent_caption_text = std::get<4>(tup);
//    info_log("handle_caption_result_tup, %d %d %d", interrupted, was_cleared, active_delay_sec);

    if (was_cleared) {
        if (!active_delay_sec) {
            this->captionLinesPlainTextEdit->clear();
            this->cleared = true;
        }

        return;
    }

    if (!caption_result)
        return;

    latest_caption_result = caption_result;
    latest_caption_text_history = recent_caption_text;
    this->cleared = false;
}


void MainCaptionWidget::handle_caption_data_cb(
        shared_ptr<OutputCaptionResult> caption_result,
        bool interrupted,
        bool cleared,
        int active_delay_sec,
        string recent_caption_text) {
    //called from different thread

//    info_log("emitttttttttttttttttttt");
    result_queue.enqueue(ResultTup(caption_result, interrupted, cleared, active_delay_sec, recent_caption_text));
    emit process_item_queue();
}

void MainCaptionWidget::menu_button_clicked() {
    show();
    raise();
}

void MainCaptionWidget::save_event_cb(obs_data_t *save_data) {
    save_obs_CaptionerSettings(save_data, current_settings);
}

void MainCaptionWidget::show_settings() {
    info_log("show_settings");

    caption_settings_widget.set_settings(current_settings);


    caption_settings_widget.show();
    caption_settings_widget.raise();
}

void MainCaptionWidget::accept_widget_settings(CaptionerSettings new_settings) {
    info_log("accept_widget_settings %p", &caption_settings_widget);
    caption_settings_widget.hide();

    apply_changed_settings(new_settings);
}

void MainCaptionWidget::set_settings(CaptionerSettings new_settings) {
    apply_changed_settings(new_settings);
    caption_settings_widget.hide();
}

MainCaptionWidget::~MainCaptionWidget() {
    captioner.clear_settings(false);
}

void MainCaptionWidget::external_state_changed() {
    apply_changed_settings(current_settings);
}

void MainCaptionWidget::handle_audio_capture_status_change(const int new_status) {
    info_log("handle_audio_capture_status_changehandle_audio_capture_status_change %d", new_status);
    string text;
    if (new_status == AUDIO_SOURCE_CAPTURING)
        text = "🔴 Captioning";
    else if (new_status == AUDIO_SOURCE_MUTED)
        text = "Muted";
    else if (new_status == AUDIO_SOURCE_NOT_STREAMED)
        text = "Source not streamed";
    else
        text = "Off";

    this->statusTextLabel->setText(text.c_str());
    this->statusTextLabel->setVisible(true);
}


void MainCaptionWidget::enabled_state_checkbox_changed(int new_checkbox_state) {
    info_log("enabled_state_changed");
    this->set_enabled(!!new_checkbox_state);
}

void MainCaptionWidget::set_enabled(bool is_enabled) {
    CaptionerSettings new_settings = current_settings;
    new_settings.enabled = is_enabled;

    current_settings.print();
    apply_changed_settings(new_settings);
    current_settings.print();
}

