//
// Created by Rat on 08.10.19.
//

#ifndef OBS_GOOGLE_CAPTION_PLUGIN_CAPTIONDOCK_H
#define OBS_GOOGLE_CAPTION_PLUGIN_CAPTIONDOCK_H

#include "../SourceCaptioner.h"
#include "../CaptionPluginManager.h"
#include "MainCaptionWidget.h"
#include "ui_CaptionDock.h"

class CaptionDock : public QWidget, Ui_CaptionDock {
Q_OBJECT
private:
    CaptionPluginManager &plugin_manager;
    MainCaptionWidget &main_caption_widget;
    string last_output_line;

    void handle_caption_data_cb(
            shared_ptr<OutputCaptionResult> caption_result,
            bool cleared,
            string recent_caption_text
    );

private slots:

    void on_settingsToolButton_clicked();

public:
    CaptionDock(const QString &title, CaptionPluginManager &plugin_manager, MainCaptionWidget &main_caption_widget);

    void handle_source_capture_status_change(shared_ptr<SourceCaptionerStatus> status);

};


#endif //OBS_GOOGLE_CAPTION_PLUGIN_CAPTIONDOCK_H
