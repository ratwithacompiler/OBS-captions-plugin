//
// Created by Rat on 08.10.19.
//

#ifndef OBS_GOOGLE_CAPTION_PLUGIN_CAPTIONDOCK_H
#define OBS_GOOGLE_CAPTION_PLUGIN_CAPTIONDOCK_H

#include <QDockWidget>
#include "../SourceCaptioner.h"
#include "../CaptionPluginManager.h"
#include "ui_CaptionDock.h"

class CaptionDock : public QDockWidget, Ui_CaptionDock {
private:
    CaptionPluginManager &plugin_manager;
    string last_output_line;

    void handle_caption_data_cb(
            shared_ptr<OutputCaptionResult> caption_result,
            bool interrupted,
            bool cleared,
            string recent_caption_text
    );

public:
    CaptionDock(const QString &title, CaptionPluginManager &plugin_manager);

    void handle_source_capture_status_change(shared_ptr<SourceCaptionerStatus> status);

};


#endif //OBS_GOOGLE_CAPTION_PLUGIN_CAPTIONDOCK_H
