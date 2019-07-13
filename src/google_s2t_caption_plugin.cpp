#include <obs.hpp>
#include <obs.h>
#include <obs-module.h>
#include <obs-frontend-api.h>
#include <obs-properties.h>

#include "SourceCaptioner.h"

#include <QAction>
#include <QMessageBox>

#include <media-io/audio-resampler.h>
#include <fstream>
#include <thread>

#include "ui/MainCaptionWidget.h"
#include "caption_stream_helper.cpp"

#include "log.c"

using namespace std;

//SourceCaptioner *captioner_instance = nullptr;
MainCaptionWidget *main_caption_widget = nullptr;

OBS_DECLARE_MODULE()

//OBS_MODULE_USE_DEFAULT_LOCALE("my-plugin", "en-US")
void finished_loading_event();

void stream_started_event();

void stream_stopped_event();

void setup_UI();

void closed_caption_tool_menu_clicked();

void obs_frontent_exiting();

static void obs_event(enum obs_frontend_event event, void *) {
    int tid = std::hash<std::thread::id>{}(std::this_thread::get_id());
    info_log("obs event %d", tid);
    if (event == OBS_FRONTEND_EVENT_FINISHED_LOADING) {
        finished_loading_event();
    } else if (event == OBS_FRONTEND_EVENT_STREAMING_STARTED) {
        stream_started_event();
    } else if (event == OBS_FRONTEND_EVENT_STREAMING_STOPPED) {
        stream_stopped_event();
    } else if (event == OBS_FRONTEND_EVENT_EXIT) {
        obs_frontent_exiting();
    } else if (event == OBS_FRONTEND_EVENT_STUDIO_MODE_ENABLED) {
        printf("studio mode!!!!!!!!!!!!!!!!!!!!\n");

    }
}


void closed_caption_tool_menu_clicked() {
    int tid = std::hash<std::thread::id>{}(std::this_thread::get_id());
    info_log("test clicked %d", tid);
    info_log("test clicked ");
    if (main_caption_widget) {
        main_caption_widget->menu_button_clicked();
    }
}

void finished_loading_event() {
    info_log("OBS_FRONTEND_EVENT_FINISHED_LOADING");
//    printf("OBS_FRONTEND_EVENT_FINISHED_LOADING\n");
    if (main_caption_widget) {
        setup_UI();

#ifdef USE_DEVMODE
        main_caption_widget->show();
#endif
    }
}

void stream_started_event() {
    info_log("stream_started_event");
    if (main_caption_widget)
        main_caption_widget->external_state_changed();
}

void stream_stopped_event() {
    info_log("stream_stopped_event");
    if (main_caption_widget)
        main_caption_widget->external_state_changed();
}

void obs_frontent_exiting() {
    info_log("obs_frontent_exiting, stopping captioner");

    if (main_caption_widget) {
//        main_caption_widget->stop();
        delete main_caption_widget;
        main_caption_widget = nullptr;
    }

}

static void save_or_load_event_callback(obs_data_t *save_data, bool saving, void *) {
    int tid = std::hash<std::thread::id>{}(std::this_thread::get_id());
    info_log("save_or_load_event_callback %d, %d", saving, tid);

    if (saving && main_caption_widget) {
        main_caption_widget->save_event_cb(save_data);


    }

    if (!saving) {
        auto loaded_settings = load_obs_CaptionerSettings(save_data);
        if (main_caption_widget) {
            main_caption_widget->set_settings(loaded_settings);
        } else {
            main_caption_widget = new MainCaptionWidget(loaded_settings);
        }
    }

}


void setup_UI() {
//    printf("main setup_UI()\n");
    QAction *action = (QAction *) obs_frontend_add_tools_menu_qaction("Cloud Closed Captions");
    action->connect(action, &QAction::triggered, &closed_caption_tool_menu_clicked);

//    setupDock();
}


bool obs_module_load(void) {
    info_log("hiiiiiiiiiiiiiiiiiiiii obs_module_load");
    p_libsys_init();
    qRegisterMetaType<std::string>();
    qRegisterMetaType<shared_ptr<CaptionResult>>();

    obs_frontend_add_event_callback(obs_event, nullptr);
    obs_frontend_add_save_callback(save_or_load_event_callback, nullptr);

    return true;
}

void obs_module_post_load(void) {
    info_log("hwoooooooooooooooo obs_module_post_load");


}

void obs_module_unload(void) {
    info_log("hwoooooooooooooooo obs_module_unload");
}