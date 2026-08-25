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

#include <obs.hpp>
#include <obs.h>
#include <obs-module.h>
#include <obs-frontend-api.h>
#include <obs-properties.h>

#include "SourceCaptioner.h"

#include <QAction>
#include <QMessageBox>
#include <QMainWindow>

#include <media-io/audio-resampler.h>
#include <fstream>
#include <functional>
#include <thread>

#include "ui/MainCaptionWidget.h"
#include "caption_stream_helper.cpp"
#include "CaptionPluginManager.h"
#include "ui/CaptionDock.h"

#include "log.h"
#include <algorithm>
#include <cstring>

using namespace std;

//SourceCaptioner *captioner_instance = nullptr;
MainCaptionWidget *main_caption_widget = nullptr;
CaptionPluginManager *plugin_manager = nullptr;

CaptionDock *caption_dock = nullptr;
bool frontend_loading_finished = false;
bool ui_setup_done = false;

static obs_hotkey_id captions_enable_hotkey_id = OBS_INVALID_HOTKEY_ID;
static obs_hotkey_id captions_disable_hotkey_id = OBS_INVALID_HOTKEY_ID;
static obs_hotkey_id captions_toggle_hotkey_id = OBS_INVALID_HOTKEY_ID;

OBS_DECLARE_MODULE()


//OBS_MODULE_USE_DEFAULT_LOCALE("my-plugin", "en-US")
void finished_loading_event();

void stream_started_event();

void stream_stopped_event();

void recording_started_event();

void recording_stopped_event();

void virtualcam_started_event();

void virtualcam_stopped_event();

void setup_UI();

void closed_caption_tool_menu_clicked();

void obs_frontent_exiting();

void obs_frontent_scene_collection_changed();

void obs_frontent_scene_collection_changing(const char *event_name);

static void unregister_hotkeys();

static void obs_event(enum obs_frontend_event event, void *) {
//    debug_log("obs_event %d", (int) std::hash<std::thread::id>{}(std::this_thread::get_id()));
//    int tid = std::hash<std::thread::id>{}(std::this_thread::get_id());
//    info_log("obs event %d", tid);
    if (event == OBS_FRONTEND_EVENT_FINISHED_LOADING) {
        finished_loading_event();
    } else if (event == OBS_FRONTEND_EVENT_STREAMING_STARTED) {
        stream_started_event();
    } else if (event == OBS_FRONTEND_EVENT_STREAMING_STOPPED) {
        stream_stopped_event();
    } else if (event == OBS_FRONTEND_EVENT_RECORDING_STARTED) {
        recording_started_event();
    } else if (event == OBS_FRONTEND_EVENT_RECORDING_STOPPED) {
        recording_stopped_event();
    } else if (event == OBS_FRONTEND_EVENT_VIRTUALCAM_STARTED) {
        virtualcam_started_event();
    } else if (event == OBS_FRONTEND_EVENT_VIRTUALCAM_STOPPED) {
        virtualcam_stopped_event();
    } else if (event == OBS_FRONTEND_EVENT_EXIT) {
        obs_frontent_exiting();
    } else if (event == OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGED) {
        obs_frontent_scene_collection_changed();
    } else if (event == OBS_FRONTEND_EVENT_STUDIO_MODE_ENABLED) {
    } else if (event == OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGING) {
        obs_frontent_scene_collection_changing("SCENE_COLLECTION_CHANGING");
    } else if (event == OBS_FRONTEND_EVENT_SCENE_COLLECTION_CLEANUP) {
        // Duplicate/Rename of scene collection sends CLEANUP not CHANGING event.
        // Need to release all sources or get obs shutdown error thing.
        obs_frontent_scene_collection_changing("SCENE_COLLECTION_CLEANUP");
    }
}


void closed_caption_tool_menu_clicked() {
    debug_log("caption menu button clicked ");
//    int tid = std::hash<std::thread::id>{}(std::this_thread::get_id());
//    info_log("test clicked %d", tid);

    if (main_caption_widget) {
        main_caption_widget->show_self();
    }
}

void setup_dock() {
    debug_log("setup_dock()");
    if (caption_dock || !plugin_manager || !main_caption_widget)
        return;

    caption_dock = new CaptionDock("Captions", *plugin_manager, *main_caption_widget);
    caption_dock->setObjectName("cloud_caption_caption_dock");

    obs_frontend_add_dock_by_id("cloud_closed_captions_dock_main", "Captions", caption_dock);
}

void setup_UI() {
    if (ui_setup_done)
        return;

    debug_log("setup_UI()");
    QAction *action = (QAction *) obs_frontend_add_tools_menu_qaction("Cloud Closed Captions");
    action->connect(action, &QAction::triggered, &closed_caption_tool_menu_clicked);

    setup_dock();

    ui_setup_done = true;
}

void finished_loading_event() {
#if  defined(__linux__)
    auto hmm = obs_output_output_caption_text2;
#endif

    frontend_loading_finished = true;

    info_log("OBS_FRONTEND_EVENT_FINISHED_LOADING, plugin_manager loaded: %d, %s",
             plugin_manager != nullptr, qVersion());
    if (main_caption_widget) {
        main_caption_widget->external_state_changed();
#ifdef USE_DEVMODE
        main_caption_widget->show();
//        main_caption_widget->stream_started_event();
#endif
    }
}

void stream_started_event() {
    info_log("stream_started_event");
    if (main_caption_widget)
        main_caption_widget->stream_started_event();
}

void stream_stopped_event() {
    info_log("stream_stopped_event");
    if (main_caption_widget)
        main_caption_widget->stream_stopped_event();
}

void recording_started_event() {
    info_log("recording_started_event");
    if (main_caption_widget)
        main_caption_widget->recording_started_event();
}

void recording_stopped_event() {
    info_log("recording_stopped_event");
    if (main_caption_widget)
        main_caption_widget->recording_stopped_event();
}

void virtualcam_started_event() {
    if (main_caption_widget)
        main_caption_widget->virtualcam_started_event();
}
void virtualcam_stopped_event() {
    if (main_caption_widget)
        main_caption_widget->virtualcam_stopped_event();
}

void obs_frontent_scene_collection_changing(const char *event_name) {
    info_log("obs_frontent_scene_collection_changing (%s), stopping captioning", event_name);
    if (main_caption_widget) {
        main_caption_widget->stop_captioning();
    }
}

void obs_frontent_scene_collection_changed() {
    info_log("obs_frontent_scene_collection_changed");
    if (main_caption_widget) {
        main_caption_widget->scene_collection_changed();
    }

}

void obs_frontent_exiting() {
    info_log("obs_frontent_exiting, stopping captioner");

    unregister_hotkeys();

    if (main_caption_widget) {
//        main_caption_widget->stop();
        delete main_caption_widget;
        main_caption_widget = nullptr;
    }

    if (plugin_manager) {
//        save_CaptionPluginSettings_to_config(plugin_manager->plugin_settings);

        delete plugin_manager;
        plugin_manager = nullptr;
    }
    debug_log("obs_frontent_exiting done");
}

//static void save_or_load_event_callback_config(obs_data_t *_, bool saving, void *) {
//    int tid = std::hash<std::thread::id>{}(std::this_thread::get_id());
//    info_log("google_s2t_caption_plugin save_or_load_event_callback %d, %d", saving, tid);
//
//    if (saving) {
//        // always save too when main OBS saves
//        if (plugin_manager) {
//            save_CaptionPluginSettings_to_config(plugin_manager->plugin_settings);
//        }
//    } else {
//        if (!plugin_manager && !main_caption_widget) {
//            info_log("google_s2t_caption_plugin initial load");
//            CaptionPluginSettings settings = load_CaptionPluginSettings_from_config();
//
//            plugin_manager = new CaptionPluginManager(settings);
//            main_caption_widget = new MainCaptionWidget(*plugin_manager);
//            setup_UI();
//        }
//    }
//}


// from hotkey thread
static void enqueue_on_ui(const std::function<void(CaptionPluginManager &)> &fn) {
    CaptionPluginManager *manager = plugin_manager;
    if (!manager)
        return;

    QMetaObject::invokeMethod(manager, [fn]() {
        if (plugin_manager)
            fn(*plugin_manager);
    }, Qt::QueuedConnection);
}

static void captions_enable_hotkey_cb(void *, obs_hotkey_id, obs_hotkey_t *, bool pressed) {
    if (pressed)
        enqueue_on_ui([](CaptionPluginManager &manager) { manager.set_enabled(true); });
}

static void captions_disable_hotkey_cb(void *, obs_hotkey_id, obs_hotkey_t *, bool pressed) {
    if (pressed)
        enqueue_on_ui([](CaptionPluginManager &manager) { manager.set_enabled(false); });
}

static void captions_toggle_hotkey_cb(void *, obs_hotkey_id, obs_hotkey_t *, bool pressed) {
    if (pressed)
        enqueue_on_ui([](CaptionPluginManager &manager) { manager.toggle_enabled(); });
}

static void register_hotkeys() {
    if (captions_enable_hotkey_id != OBS_INVALID_HOTKEY_ID)
        return;

    captions_enable_hotkey_id = obs_hotkey_register_frontend(
            "cloud_closed_captions.enable", "Enable Captions", captions_enable_hotkey_cb, nullptr);
    captions_disable_hotkey_id = obs_hotkey_register_frontend(
            "cloud_closed_captions.disable", "Disable Captions", captions_disable_hotkey_cb, nullptr);
    captions_toggle_hotkey_id = obs_hotkey_register_frontend(
            "cloud_closed_captions.toggle", "Toggle Captions", captions_toggle_hotkey_cb, nullptr);
}

static void unregister_hotkeys() {
    obs_hotkey_unregister(captions_enable_hotkey_id);
    obs_hotkey_unregister(captions_disable_hotkey_id);
    obs_hotkey_unregister(captions_toggle_hotkey_id);
    captions_enable_hotkey_id = OBS_INVALID_HOTKEY_ID;
    captions_disable_hotkey_id = OBS_INVALID_HOTKEY_ID;
    captions_toggle_hotkey_id = OBS_INVALID_HOTKEY_ID;
}

static obs_data_t *get_hotkey_save_data() {
    obs_data_t *data = obs_data_create();

    obs_data_array_t *enable_arr = obs_hotkey_save(captions_enable_hotkey_id);
    obs_data_set_array(data, "enable", enable_arr);
    obs_data_array_release(enable_arr);

    obs_data_array_t *disable_arr = obs_hotkey_save(captions_disable_hotkey_id);
    obs_data_set_array(data, "disable", disable_arr);
    obs_data_array_release(disable_arr);

    obs_data_array_t *toggle_arr = obs_hotkey_save(captions_toggle_hotkey_id);
    obs_data_set_array(data, "toggle", toggle_arr);
    obs_data_array_release(toggle_arr);

    return data;
}

static void load_hotkeys(obs_data_t *data) {
    if (!data)
        return;

    obs_data_array_t *enable_arr = obs_data_get_array(data, "enable");
    obs_hotkey_load(captions_enable_hotkey_id, enable_arr);
    obs_data_array_release(enable_arr);

    obs_data_array_t *disable_arr = obs_data_get_array(data, "disable");
    obs_hotkey_load(captions_disable_hotkey_id, disable_arr);
    obs_data_array_release(disable_arr);

    obs_data_array_t *toggle_arr = obs_data_get_array(data, "toggle");
    obs_hotkey_load(captions_toggle_hotkey_id, toggle_arr);
    obs_data_array_release(toggle_arr);
}

static void save_or_load_event_callback(obs_data_t *save_data, bool saving, void *) {
    int tid = std::hash<std::thread::id>{}(std::this_thread::get_id());
    debug_log("save_or_load_event_callback %d, %d", saving, tid);

    if (saving && plugin_manager) {
        save_CaptionPluginSettings(save_data, plugin_manager->plugin_settings);

        if (captions_enable_hotkey_id != OBS_INVALID_HOTKEY_ID) {
            obs_data_t *hotkey_data = get_hotkey_save_data();
            set_additional_save_data(save_data, "hotkey", hotkey_data);
            obs_data_release(hotkey_data);
        }
    }

    if (!saving) {
        auto loaded_settings = load_CaptionPluginSettings(save_data);
        if (plugin_manager && main_caption_widget) {
            plugin_manager->update_settings(loaded_settings);
        } else if (plugin_manager || main_caption_widget) {
            error_log("only one of plugin_manager and main_caption_widget, wtf, %d %d",
                      plugin_manager != nullptr, main_caption_widget != nullptr);
        } else {
            plugin_manager = new CaptionPluginManager(loaded_settings);
            main_caption_widget = new MainCaptionWidget(*plugin_manager);
            setup_UI();
            register_hotkeys();
        }

        obs_data_t *hotkey_data = get_additional_save_data(save_data, "hotkey");
        load_hotkeys(hotkey_data);
        obs_data_release(hotkey_data);
    }

}


static void blog_log_sink(int level, const char *line) {
    int obs_level = LOG_INFO;
    if (level == CAPTIONS_LOG_ERROR)
        obs_level = LOG_ERROR;
    else if (level == CAPTIONS_LOG_WARNING)
        obs_level = LOG_WARNING;
    else if (level == CAPTIONS_LOG_DEBUG)
        obs_level = LOG_DEBUG;

    const size_t max_chunk = 7900;
    size_t total = strlen(line);
    if (total <= max_chunk) {
        blog(obs_level, "%s", line);
        return;
    }

    size_t chunk_count = (total + max_chunk - 1) / max_chunk;
    for (size_t i = 0; i < chunk_count; i++) {
        std::string chunk(line + i * max_chunk, std::min(max_chunk, total - i * max_chunk));
        blog(obs_level, "(%d/%d) %s", (int) (i + 1), (int) chunk_count, chunk.c_str());
    }
}


bool obs_module_load(void) {
    captions_log_sink.store(blog_log_sink, std::memory_order_relaxed);
    info_log("%s obs_module_load %d", VERSION_STRING,
             (int) std::hash<std::thread::id>{}(std::this_thread::get_id()));
    qRegisterMetaType<std::string>();
    qRegisterMetaType<shared_ptr<OutputCaptionResult>>();
    qRegisterMetaType<CaptionResult>();
    qRegisterMetaType<std::shared_ptr<SourceCaptionerStatus>>();

    obs_frontend_add_event_callback(obs_event, nullptr);
    obs_frontend_add_save_callback(save_or_load_event_callback, nullptr);
    return true;
}

void obs_module_post_load(void) {
    info_log("%s obs_module_post_load", VERSION_STRING);
}

void obs_module_unload(void) {
    info_log("%s obs_module_unload", VERSION_STRING);
    captions_log_sink.store(captions_log_stderr_sink, std::memory_order_relaxed);
}

MODULE_EXPORT const char *obs_module_description(void)
{
    return "Provides closed captioning via Google Cloud Speech Recognition API";
}

MODULE_EXPORT const char *obs_module_name(void)
{
    return "Cloud Closed Captions";
}

