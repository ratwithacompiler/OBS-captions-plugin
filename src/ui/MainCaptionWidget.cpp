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
#include "uiutils.h"

MainCaptionWidget::MainCaptionWidget(CaptionPluginManager &plugin_manager) :
        QWidget(),
        Ui_MainCaptionWidget(),
        plugin_manager(plugin_manager),
        caption_settings_widget(plugin_manager.plugin_settings) {
//    debug_log("MainCaptionWidget");
    setupUi(this);

    this->captionHistoryPlainTextEdit->setPlainText("");
    this->captionLinesPlainTextEdit->setPlainText("");

    QObject::connect(this->enabledCheckbox, &QCheckBox::stateChanged, this, &MainCaptionWidget::enabled_state_checkbox_changed);
    QObject::connect(this->settingsToolButton, &QToolButton::clicked, this, &MainCaptionWidget::show_settings);
    statusTextLabel->hide();

    QObject::connect(this, &MainCaptionWidget::process_item_queue, this, &MainCaptionWidget::do_process_item_queue);

    QObject::connect(&caption_settings_widget, &CaptionSettingsWidget::settings_accepted,
                     this, &MainCaptionWidget::accept_widget_settings);

    QObject::connect(&plugin_manager.source_captioner, &SourceCaptioner::caption_result_received,
                     this, &MainCaptionWidget::handle_caption_data_cb, Qt::QueuedConnection);

    QObject::connect(&plugin_manager.source_captioner, &SourceCaptioner::source_capture_status_changed,
                     this, &MainCaptionWidget::handle_source_capture_status_change, Qt::QueuedConnection);

    QObject::connect(&plugin_manager, &CaptionPluginManager::settings_changed,
                     this, &MainCaptionWidget::settings_changed_event);

    settings_changed_event(plugin_manager.plugin_settings);
}


void MainCaptionWidget::showEvent(QShowEvent *event) {
    debug_log("MainCaptionWidget show event");
    QWidget::showEvent(event);
    update_caption_text_ui();
    external_state_changed();
}

void MainCaptionWidget::hideEvent(QHideEvent *event) {
    debug_log("MainCaptionWidget hide event");
    QWidget::hideEvent(event);

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

        this->captionLinesPlainTextEdit->setPlainText(QString::fromStdString(single_caption_line));
    }
}


void MainCaptionWidget::handle_caption_result_tup(ResultTup &tup) {
    auto caption_result = std::get<0>(tup);
//    bool interrupted = std::get<1>(tup);
    bool was_cleared = std::get<2>(tup);
    string recent_caption_text = std::get<3>(tup);
//    info_log("handle_caption_result_tup, %d %d", interrupted, was_cleared);

    if (was_cleared) {
        this->captionLinesPlainTextEdit->clear();
        this->cleared = true;

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
        string recent_caption_text) {
    result_queue.enqueue(ResultTup(caption_result, interrupted, cleared, recent_caption_text));
    emit process_item_queue();
}

void MainCaptionWidget::menu_button_clicked() {
    show();
    raise();
}

void MainCaptionWidget::show_settings() {
    debug_log("MainCaptionWidget show_settings");
    caption_settings_widget.set_settings(plugin_manager.plugin_settings);

    caption_settings_widget.show();
    caption_settings_widget.raise();
}

void MainCaptionWidget::accept_widget_settings(CaptionPluginSettings new_settings) {
    debug_log("MainCaptionWidget accept_widget_settings %p", &caption_settings_widget);
    caption_settings_widget.hide();
    plugin_manager.update_settings(new_settings);
}

void MainCaptionWidget::external_state_changed() {
    plugin_manager.external_state_changed(is_stream_live(), isVisible(), is_recording_live());
}

MainCaptionWidget::~MainCaptionWidget() {
    plugin_manager.source_captioner.stop_caption_stream(false);
}

void MainCaptionWidget::stream_started_event() {
    plugin_manager.source_captioner.stream_started_event();
    external_state_changed();
}

void MainCaptionWidget::stream_stopped_event() {
    plugin_manager.source_captioner.stream_stopped_event();
    external_state_changed();
}

void MainCaptionWidget::recording_started_event() {
    plugin_manager.source_captioner.recording_started_event();
    external_state_changed();
}

void MainCaptionWidget::recording_stopped_event() {
    plugin_manager.source_captioner.recording_stopped_event();
    external_state_changed();
}

void MainCaptionWidget::handle_source_capture_status_change(shared_ptr<SourceCaptionerStatus> status) {
//    info_log("handle_source_capture_status_change");
    if (!status)
        return;

    string text;
    captioning_status_string(plugin_manager.plugin_settings.enabled, *status, text);

    this->statusTextLabel->setText(text.c_str());
    this->statusTextLabel->setVisible(true);
}


void MainCaptionWidget::enabled_state_checkbox_changed(int new_checkbox_state) {
    debug_log("MainCaptionWidget enabled_state_checkbox_changed");
    plugin_manager.toggle_enabled();
}

void MainCaptionWidget::settings_changed_event(CaptionPluginSettings new_settings) {
    debug_log("MainCaptionWidget settings_changed_event");

    if (new_settings.enabled != enabledCheckbox->isChecked()) {
        const QSignalBlocker blocker(enabledCheckbox);
        enabledCheckbox->setChecked(new_settings.enabled);
    }

//    latest_caption_result = nullptr;
//    latest_caption_text_history.clear();
//    update_caption_text_ui();
}
