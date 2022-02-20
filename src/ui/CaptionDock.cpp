//
// Created by Rat on 08.10.19.
//

#include "CaptionDock.h"
#include <QLabel>
#include <QLineEdit>
#include "../log.c"
#include "uiutils.h"

CaptionDock::CaptionDock(const QString &title, CaptionPluginManager &plugin_manager, MainCaptionWidget &main_caption_widget)
        : QDockWidget(title), plugin_manager(plugin_manager), main_caption_widget(main_caption_widget) {
    setupUi(this);
    setWindowTitle(title);
    captionLinesPlainTextEdit->clear();

    setFeatures(QDockWidget::AllDockWidgetFeatures);
    setFloating(true);

    QObject::connect(&plugin_manager.source_captioner, &SourceCaptioner::source_capture_status_changed,
                     this, &CaptionDock::handle_source_capture_status_change, Qt::QueuedConnection);

    QObject::connect(&plugin_manager.source_captioner, &SourceCaptioner::caption_result_received,
                     this, &CaptionDock::handle_caption_data_cb, Qt::QueuedConnection);

    QFontMetrics fm = this->captionLinesPlainTextEdit->fontMetrics();
    info_log("dock: %d %d fs: %d", this->minimumWidth(), this->maximumWidth(), this->captionLinesPlainTextEdit->font().pointSize());

//    const int target_width = fm.width("This is a baseline example test okay") + 0;
    const int target_width = fm.width("THIS IS A BASELINE EXAMPLE TEST OKAY") + 30;
    info_log("target: %d %d", target_width, 0);
    if (target_width >= 150 && target_width <= 350) {
        this->setMaximumWidth(target_width);
        this->setMinimumWidth(target_width);
    }
    this->verticalLayout->setAlignment(Qt::AlignTop);
//    info_log("after: %d %d", this->minimumWidth(), this->maximumWidth());
}

void CaptionDock::handle_source_capture_status_change(shared_ptr<SourceCaptionerStatus> status) {
    if (!status)
        return;

//    debug_log("CaptionDock::handle_source_capture_status_change %d", status->audio_capture_status);
    string status_text;
    captioning_status_string(
            plugin_manager.plugin_settings.enabled,
            status->settings.streaming_output_enabled,
            status->settings.recording_output_enabled,
            plugin_manager.captioning_state(),
            *status,
            status_text);

    this->statusTextLabel->setText(QString::fromStdString(status_text));
}

void CaptionDock::handle_caption_data_cb(
        shared_ptr<OutputCaptionResult> caption_result,
        bool cleared,
        string recent_caption_text
) {
    if (cleared) {
        captionLinesPlainTextEdit->setPlainText("");
        last_output_line = "";
        return;
    }

    if (!caption_result)
        return;

//    debug_log("%d %s", caption_result->output_line == last_output_line, caption_result->output_line.c_str());
    if (caption_result->output_line == last_output_line) {
//        debug_log("ignoring dup %s", caption_result->output_line.c_str());
        return;
    }

    string single_caption_line;
    for (string &a_line: caption_result->output_lines) {
        if (!single_caption_line.empty())
            single_caption_line.push_back('\n');

        single_caption_line.append(a_line);
    }

    this->captionLinesPlainTextEdit->setPlainText(QString::fromStdString(single_caption_line));
    last_output_line = caption_result->output_line;
}

void CaptionDock::on_settingsToolButton_clicked() {
//    debug_log("on_settingsToolButton_clicked");
    main_caption_widget.show_settings_dialog();
}
