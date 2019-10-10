//
// Created by Rat on 08.10.19.
//

#include "CaptionDock.h"
#include <QLabel>
#include <QLineEdit>
#include "../log.c"
#include "uiutils.h"

CaptionDock::CaptionDock(const QString &title, CaptionPluginManager &plugin_manager)
        : QDockWidget(title), plugin_manager(plugin_manager) {
    setupUi(this);
    setWindowTitle(title);
    captionLinesPlainTextEdit->clear();

    setFeatures(QDockWidget::AllDockWidgetFeatures);
    setFloating(true);


    QObject::connect(&plugin_manager.source_captioner, &SourceCaptioner::source_capture_status_changed,
                     this, &CaptionDock::handle_source_capture_status_change, Qt::QueuedConnection);

    QObject::connect(&plugin_manager.source_captioner, &SourceCaptioner::caption_result_received,
                     this, &CaptionDock::handle_caption_data_cb, Qt::QueuedConnection);
}

void CaptionDock::handle_source_capture_status_change(shared_ptr<SourceCaptionerStatus> status) {
//    debug_log("CaptionDock::handle_source_capture_status_change %d", status->audio_capture_status);
    string status_text;
    captioning_status_string(
            plugin_manager.plugin_settings.enabled,
            *status,
            plugin_manager.plugin_settings.source_cap_settings.caption_source_settings.caption_source_name,
            status_text);

    this->statusTextLabel->setText(QString::fromStdString(status_text));
}

void CaptionDock::handle_caption_data_cb(
        shared_ptr<OutputCaptionResult> caption_result,
        bool interrupted, bool cleared,
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
