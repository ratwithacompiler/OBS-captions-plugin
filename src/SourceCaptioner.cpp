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

#include <memory>


#include "SourceCaptioner.h"
#include "log.c"
#include "caption_output_writer.h"
#include "caption_transcript_writer.h"


void set_text_source_text(const string &text_source_name, const string &caption_text) {
    obs_source_t *text_source = obs_get_source_by_name(text_source_name.c_str());
    if (!text_source) {
//        warn_log("text source: %s not found, can't set caption text", text_source_name.c_str());
        return;
    }
//    debug_log("set_text_source_text '%s': '%s'", text_source_name.c_str(), caption_text.c_str());

    obs_data_t *text_settings = obs_data_create();
    obs_data_set_string(text_settings, "text", caption_text.c_str());
    obs_source_update(text_source, text_settings);
    obs_data_release(text_settings);

    obs_source_release(text_source);
}

SourceCaptioner::SourceCaptioner(const SourceCaptionerSettings &settings, const string &scene_collection_name, bool start) :
        QObject(),
        settings(settings),
        selected_scene_collection_name(scene_collection_name),
        last_caption_at(std::chrono::steady_clock::now()),
        last_caption_cleared(true) {

    QObject::connect(&timer, &QTimer::timeout, this, &SourceCaptioner::clear_output_timer_cb);

    QObject::connect(this, &SourceCaptioner::received_caption_result,
                     this, &SourceCaptioner::process_caption_result, Qt::QueuedConnection);

    QObject::connect(this, &SourceCaptioner::audio_capture_status_changed,
                     this, &SourceCaptioner::process_audio_capture_status_change);

    timer.start(1000);

    const SceneCollectionSettings &scene_col_settings = this->settings.get_scene_collection_settings(scene_collection_name);
    debug_log("SourceCaptioner, source '%s'", scene_col_settings.caption_source_settings.caption_source_name.c_str());

    if (start)
        start_caption_stream(settings, scene_collection_name);
}


void SourceCaptioner::stop_caption_stream(bool send_signal) {
    if (!send_signal) {
        std::lock_guard<recursive_mutex> lock(settings_change_mutex);
        source_audio_capture_session = nullptr;
        output_audio_capture_session = nullptr;
        caption_result_handler = nullptr;
        continuous_captions = nullptr;
        audio_capture_id++;
        return;
    }

    settings_change_mutex.lock();

    SourceCaptionerSettings cur_settings = settings;
    string cur_scene_collection_name = this->selected_scene_collection_name;

    source_audio_capture_session = nullptr;
    output_audio_capture_session = nullptr;
    caption_result_handler = nullptr;
    continuous_captions = nullptr;
    audio_capture_id++;

    settings_change_mutex.unlock();

    if (send_signal) {
        emit source_capture_status_changed(std::make_shared<SourceCaptionerStatus>(
                SOURCE_CAPTIONER_STATUS_EVENT_STOPPED,
                false,
                false,
                cur_settings,
                cur_scene_collection_name,
                AUDIO_SOURCE_NOT_STREAMED,
                false
        ));
    }
}

bool SourceCaptioner::set_settings(const SourceCaptionerSettings &new_settings, const string &scene_collection_name) {
//    debug_log("SourceCaptioner::set_settings");

    bool settings_equal;
    bool stream_settings_equal;
    {
        std::lock_guard<recursive_mutex> lock(settings_change_mutex);
        stop_caption_stream(false);

        settings_equal = settings == new_settings;
        stream_settings_equal = settings.stream_settings == new_settings.stream_settings;

        settings = new_settings;
        selected_scene_collection_name = scene_collection_name;
    }

    emit source_capture_status_changed(std::make_shared<SourceCaptionerStatus>(
            SOURCE_CAPTIONER_STATUS_EVENT_NEW_SETTINGS_STOPPED,
            !settings_equal,
            !stream_settings_equal,
            new_settings,
            scene_collection_name,
            AUDIO_SOURCE_NOT_STREAMED,
            false
    ));

    return true;
}

bool SourceCaptioner::start_caption_stream(const SourceCaptionerSettings &new_settings, const string &scene_collection_name) {
//    debug_log("start_caption_stream %d", (int) std::hash<std::thread::id>{}(std::this_thread::get_id()));
    bool started_ok;
    bool settings_equal;
    bool stream_settings_equal;
    audio_source_capture_status audio_cap_status = AUDIO_SOURCE_NOT_STREAMED;

    {
        std::lock_guard<recursive_mutex> lock(settings_change_mutex);
        settings_equal = settings == new_settings;
        stream_settings_equal = settings.stream_settings == new_settings.stream_settings;

        settings = new_settings;
        selected_scene_collection_name = scene_collection_name;

        source_audio_capture_session = nullptr;
        output_audio_capture_session = nullptr;
        caption_result_handler = nullptr;
        audio_capture_id++;

        started_ok = _start_caption_stream(!stream_settings_equal);

        if (!started_ok)
            stop_caption_stream(false);

        if (started_ok) {
            if (source_audio_capture_session)
                audio_cap_status = source_audio_capture_session->get_current_capture_status();
            else if (output_audio_capture_session)
                audio_cap_status = AUDIO_SOURCE_CAPTURING;
            else {
                stop_caption_stream(false);
                started_ok = false;
            }
        }
    }

    if (started_ok) {
//        info_log("start_caption_stream OK, %d ", audio_cap_status);
        emit source_capture_status_changed(std::make_shared<SourceCaptionerStatus>(
                SOURCE_CAPTIONER_STATUS_EVENT_STARTED_OK,
                !settings_equal,
                !stream_settings_equal,
                new_settings,
                scene_collection_name,
                audio_cap_status,
                true
        ));
    } else {
//        info_log("start_caption_stream FAIL");
        emit source_capture_status_changed(std::make_shared<SourceCaptionerStatus>(
                SOURCE_CAPTIONER_STATUS_EVENT_STARTED_ERROR,
                !settings_equal,
                !stream_settings_equal,
                new_settings,
                scene_collection_name,
                AUDIO_SOURCE_NOT_STREAMED,
                false
        ));
    }

    return started_ok;
}

bool SourceCaptioner::_start_caption_stream(bool restart_stream) {
//    debug_log("start_caption_stream");

    bool caption_settings_equal;
    {
        const SceneCollectionSettings &scene_col_settings = this->settings.get_scene_collection_settings(selected_scene_collection_name);
        const CaptionSourceSettings &selected_caption_source_settings = scene_col_settings.caption_source_settings;

        debug_log("SourceCaptioner start_caption_stream, source '%s'", selected_caption_source_settings.caption_source_name.c_str());

        if (selected_caption_source_settings.caption_source_name.empty()) {
            warn_log("SourceCaptioner start_caption_stream, empty source given.");
            return false;
        }

        const bool use_streaming_output_audio = is_streaming_audio_output_capture_source_name(
                selected_caption_source_settings.caption_source_name);
        const bool use_recording_output_audio = is_recording_audio_output_capture_source_name(
                selected_caption_source_settings.caption_source_name);
        const bool use_output_audio = use_streaming_output_audio || use_recording_output_audio;

        OBSSource caption_source;
        OBSSource mute_source;

        if (!use_output_audio) {
            caption_source = obs_get_source_by_name(selected_caption_source_settings.caption_source_name.c_str());
            obs_source_release(caption_source);
            if (!caption_source) {
                warn_log("SourceCaptioner start_caption_stream, no caption source with name: '%s'",
                         selected_caption_source_settings.caption_source_name.c_str());
                return false;
            }

            if (selected_caption_source_settings.mute_when == CAPTION_SOURCE_MUTE_TYPE_USE_OTHER_MUTE_SOURCE) {
                mute_source = obs_get_source_by_name(selected_caption_source_settings.mute_source_name.c_str());
                obs_source_release(mute_source);

                if (!mute_source) {
                    warn_log("SourceCaptioner start_caption_stream, no mute source with name: '%s'",
                             selected_caption_source_settings.mute_source_name.c_str());
                    return false;
                }
            }
        }

        debug_log("caption_settings_equal: %d, %d", caption_settings_equal, continuous_captions != nullptr);
        if (!continuous_captions || restart_stream) {
            try {

#if ENABLE_CUSTOM_API_KEY
                ContinuousCaptionStreamSettings settings_copy = settings.stream_settings;
                debug_log("using ENABLE_CUSTOM_API_KEY %lu", settings_copy.stream_settings.api_key.length());
#else
#ifdef GOOGLE_API_KEY_STR
                ContinuousCaptionStreamSettings settings_copy = settings.stream_settings;
                settings_copy.stream_settings.api_key = GOOGLE_API_KEY_STR;
                debug_log("using GOOGLE_API_KEY_STR %lu", settings_copy.stream_settings.api_key.length());
#endif
#endif

                auto caption_cb = std::bind(&SourceCaptioner::on_caption_text_callback, this, std::placeholders::_1, std::placeholders::_2);
                continuous_captions = std::make_unique<ContinuousCaptions>(settings_copy);
                continuous_captions->on_caption_cb_handle.set(caption_cb, true);
            }
            catch (...) {
                warn_log("couldn't create ContinuousCaptions");
                return false;
            }
        }
        caption_result_handler = std::make_unique<CaptionResultHandler>(settings.format_settings);

        try {
            resample_info resample_to = {16000, AUDIO_FORMAT_16BIT, SPEAKERS_MONO};
            audio_chunk_data_cb audio_cb = std::bind(&SourceCaptioner::on_audio_data_callback, this,
                                                     std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

            auto audio_status_cb = std::bind(&SourceCaptioner::on_audio_capture_status_change_callback, this,
                                             std::placeholders::_1, std::placeholders::_2);

            if (use_output_audio) {
                output_audio_capture_session = std::make_unique<OutputAudioCaptureSession>(use_streaming_output_audio,
                                                                                           audio_cb, audio_status_cb,
                                                                                           resample_to,
                                                                                           audio_capture_id);
            } else {
                source_audio_capture_session = std::make_unique<SourceAudioCaptureSession>(caption_source, mute_source, audio_cb,
                                                                                           audio_status_cb,
                                                                                           resample_to,
                                                                                           MUTED_SOURCE_REPLACE_WITH_ZERO,
                                                                                           false,
                                                                                           audio_capture_id);
            }


        }
        catch (std::string err) {
            warn_log("couldn't create AudioCaptureSession, %s", err.c_str());
            return false;
        }
        catch (...) {
            warn_log("couldn't create AudioCaptureSession");
            return false;
        }

    }
//    debug_log("started captioning source '%s'", selected_caption_source_settings.caption_source_name.c_str());
//    debug_log("started captioning source tid '%d'", std::hash<std::thread::id>{}(std::this_thread::get_id()));
    return true;
}

void SourceCaptioner::on_audio_capture_status_change_callback(const int id, const audio_source_capture_status status) {
//    debug_log("capture status change %d %d", status, (int) std::hash<std::thread::id>{}(std::this_thread::get_id()));
    emit audio_capture_status_changed(id, status);
}


void SourceCaptioner::process_audio_capture_status_change(const int cb_audio_capture_id, const int new_status) {
//    debug_log("process_audio_capture_status_change %d %d", new_status, (int) std::hash<std::thread::id>{}(std::this_thread::get_id()));

    settings_change_mutex.lock();

    bool is_old_audio_session = cb_audio_capture_id != audio_capture_id;
    SourceCaptionerSettings cur_settings = settings;
    string cur_scene_collection_name = selected_scene_collection_name;
    bool active = continuous_captions != nullptr;

    settings_change_mutex.unlock();

    if (is_old_audio_session) {
        debug_log("ignoring old audio capture status!!");
        return;
    }

    emit source_capture_status_changed(std::make_shared<SourceCaptionerStatus>(
            SOURCE_CAPTIONER_STATUS_EVENT_AUDIO_CAPTURE_STATUS_CHANGE,
            false,
            false,
            cur_settings,
            cur_scene_collection_name,
            (audio_source_capture_status) new_status,
            active
    ));
}


void SourceCaptioner::on_audio_data_callback(const int id, const uint8_t *data, const size_t size) {
//    info_log("audio data");
    if (continuous_captions) {
        // safe without locking as continuous_captions only ever gets updated when there's no AudioCaptureSession running
        continuous_captions->queue_audio_data((char *) data, size);
    }
    audio_chunk_count++;

}

void SourceCaptioner::clear_output_timer_cb() {
//    info_log("clear timer checkkkkkkkkkkkkkkk");

    bool to_stream, to_recording, to_transcript_streaming, to_transcript_recording;
    string text_source_target_name;
    {
        std::lock_guard<recursive_mutex> lock(settings_change_mutex);
        if (!this->settings.format_settings.caption_timeout_enabled || this->last_caption_cleared)
            return;

        double secs_since_last_caption = std::chrono::duration_cast<std::chrono::duration<double >>(
                std::chrono::steady_clock::now() - this->last_caption_at).count();

        if (secs_since_last_caption <= this->settings.format_settings.caption_timeout_seconds)
            return;

        info_log("last caption line was sent %f secs ago, > %f, clearing",
                 secs_since_last_caption, this->settings.format_settings.caption_timeout_seconds);

        this->last_caption_cleared = true;
        to_stream = settings.streaming_output_enabled;
        to_recording = settings.recording_output_enabled;
        to_transcript_streaming = settings.transcript_settings.enabled && settings.transcript_settings.streaming_transcripts_enabled;
        to_transcript_recording = settings.transcript_settings.enabled && settings.transcript_settings.recording_transcripts_enabled;

        const SceneCollectionSettings &scene_col_settings = this->settings.get_scene_collection_settings(selected_scene_collection_name);
        if (scene_col_settings.text_output_settings.enabled
            && !scene_col_settings.text_output_settings.text_source_name.empty()) {
            text_source_target_name = scene_col_settings.text_output_settings.text_source_name;
        }

    }

    auto now = std::chrono::steady_clock::now();
    auto clearance = CaptionOutput(std::make_shared<OutputCaptionResult>(CaptionResult(0, false, 0, "", "", now, now), false), true);
    output_caption_writers(clearance,
                           to_stream,
                           to_recording,
                           false,
                           false,
                           true);

    if (!text_source_target_name.empty()) {
        set_text_source_text(text_source_target_name, "");
    }

    emit caption_result_received(nullptr, true, "");
}


void SourceCaptioner::store_result(shared_ptr<OutputCaptionResult> output_result) {
    if (!output_result)
        return;

    if (output_result->caption_result.final) {
        results_history.push_back(output_result);
        held_nonfinal_caption_result = nullptr;
        debug_log("final, adding to history: %d %s", (int) results_history.size(), output_result->clean_caption_text.c_str());
    } else {
        held_nonfinal_caption_result = output_result;
    }

    if (results_history.size() > HISTORY_ENTRIES_HIGH_WM) {
        results_history.erase(results_history.begin(), results_history.begin() + HISTORY_ENTRIES_LOW_WM);
//        debug_log("cleaning result history done %d", (int) results_history.size());
    }
}


void SourceCaptioner::prepare_recent(string &recent_captions_output) {
    for (auto i = results_history.rbegin(); i != results_history.rend(); ++i) {
        if (!(*i))
            break;

        if (!(*i)->caption_result.final)
            break;

        if (recent_captions_output.size() + (*i)->clean_caption_text.size() >= MAX_HISTORY_VIEW_LENGTH)
            break;

        if ((*i)->clean_caption_text.empty())
            continue;

        if (recent_captions_output.empty()) {
            recent_captions_output.insert(0, 1, '.');
        } else {
            recent_captions_output.insert(0, ". ");
        }

        recent_captions_output.insert(0, (*i)->clean_caption_text);
    }

    if (held_nonfinal_caption_result) {
        if (!recent_captions_output.empty())
            recent_captions_output.push_back(' ');
        recent_captions_output.append("    >> ");
        recent_captions_output.append(held_nonfinal_caption_result->clean_caption_text);
    }
}

void SourceCaptioner::on_caption_text_callback(const CaptionResult &caption_result, bool interrupted) {
    // emit qt signal to avoid possible thread deadlock
    // this callback comes from the captioner thread, result processing needs settings_change_mutex, so does clearing captioner,
    // but that waits for the captioner callback to finish which might be waiting on the lock otherwise.

    emit received_caption_result(caption_result, interrupted);
}

void SourceCaptioner::process_caption_result(const CaptionResult caption_result, bool interrupted) {
    shared_ptr<OutputCaptionResult> native_output_result;
    string recent_caption_text;
    bool to_stream, to_recording, to_transcript_streaming, to_transcript_recording;

    if (this->last_caption_text == caption_result.caption_text && this->last_caption_final == caption_result.final) {
        return;
    }
    this->last_caption_text = caption_result.caption_text;
    this->last_caption_final = caption_result.final;

    shared_ptr<OutputCaptionResult> text_output_result;
    string text_source_target_name;
    {
        std::lock_guard<recursive_mutex> lock(settings_change_mutex);

        if (!caption_result_handler) {
            warn_log("no caption_result_handler, shouldn't happen, there should be no AudioCaptureSession running");
            return;
        }

        native_output_result = caption_result_handler->prepare_caption_output(caption_result,
                                                                              true,
                                                                              settings.format_settings.caption_insert_newlines,
                                                                              settings.format_settings.caption_line_length,
                                                                              settings.format_settings.caption_line_count,
                                                                              settings.format_settings.capitalization,
                                                                              interrupted,
                                                                              results_history);
        if (!native_output_result)
            return;

        const SceneCollectionSettings &scene_col_settings = this->settings.get_scene_collection_settings(selected_scene_collection_name);
        if (scene_col_settings.text_output_settings.enabled
            && !scene_col_settings.text_output_settings.text_source_name.empty()) {

            text_source_target_name = scene_col_settings.text_output_settings.text_source_name;
            text_output_result = caption_result_handler->prepare_caption_output(caption_result,
                                                                                true,
                                                                                true,
                                                                                scene_col_settings.text_output_settings.line_length,
                                                                                scene_col_settings.text_output_settings.line_count,
                                                                                scene_col_settings.text_output_settings.capitalization,
                                                                                interrupted,
                                                                                results_history);
        }

        store_result(native_output_result);

//        info_log("got caption '%s'", native_output_result->clean_caption_text.c_str());
//        info_log("output line '%s'", output_caption_line.c_str());

        prepare_recent(recent_caption_text);

        to_stream = settings.streaming_output_enabled;
        to_recording = settings.recording_output_enabled;
        to_transcript_streaming = settings.transcript_settings.enabled && settings.transcript_settings.streaming_transcripts_enabled;
        to_transcript_recording = settings.transcript_settings.enabled && settings.transcript_settings.recording_transcripts_enabled;
    }

    this->output_caption_writers(CaptionOutput(native_output_result, false),
                                 to_stream,
                                 to_recording,
                                 to_transcript_streaming,
                                 to_transcript_recording,
                                 false);

    if (text_output_result && !text_source_target_name.empty()) {
        set_text_source_text(text_source_target_name, text_output_result->output_line);
    }

    emit caption_result_received(native_output_result, false, recent_caption_text);
}


void SourceCaptioner::output_caption_writers(
        const CaptionOutput &output,
        bool to_stream,
        bool to_recoding,
        bool to_transcript_streaming,
        bool to_transcript_recording,
        bool is_clearance) {

    bool sent_stream = false;
    if (to_stream) {
        sent_stream = streaming_output.enqueue(output);
    }

    bool sent_recording = false;
    if (to_recoding) {
        sent_recording = recording_output.enqueue(output);
    }

    bool sent_transcript_streaming = false;
    if (to_transcript_streaming) {
        sent_transcript_streaming = transcript_streaming_output.enqueue(output);
    }
    bool sent_transcript_recording = false;
    if (to_transcript_recording) {
        sent_transcript_recording = transcript_recording_output.enqueue(output);
    }

//    debug_log("queuing caption line , stream: %d, recording: %d, '%s'",
//              sent_stream, sent_recording, output.line.c_str());

    if (!is_clearance)
        caption_was_output();
}

void SourceCaptioner::caption_was_output() {
    this->last_caption_at = std::chrono::steady_clock::now();
    this->last_caption_cleared = false;
}


void SourceCaptioner::stream_started_event() {
    settings_change_mutex.lock();
    SourceCaptionerSettings cur_settings = settings;
    settings_change_mutex.unlock();

    auto control_output = std::make_shared<CaptionOutputControl<int>>(0);
    streaming_output.set_control(control_output);
    std::thread th(caption_output_writer_loop, control_output, true);
    th.detach();

    if (cur_settings.transcript_settings.enabled && cur_settings.transcript_settings.streaming_transcripts_enabled) {
        auto control_transcript = std::make_shared<CaptionOutputControl<TranscriptOutputSettings>>(cur_settings.transcript_settings);
        transcript_streaming_output.set_control(control_transcript);

        std::thread th2(transcript_writer_loop, control_transcript, true, cur_settings.transcript_settings);
        th2.detach();
    }
}

void SourceCaptioner::stream_stopped_event() {
    streaming_output.clear();
    transcript_streaming_output.clear();
}

void SourceCaptioner::recording_started_event() {
    settings_change_mutex.lock();
    SourceCaptionerSettings cur_settings = settings;
    settings_change_mutex.unlock();

    auto control_output = std::make_shared<CaptionOutputControl<int>>(0);
    recording_output.set_control(control_output);
    std::thread th(caption_output_writer_loop, control_output, false);
    th.detach();

    if (cur_settings.transcript_settings.enabled && cur_settings.transcript_settings.recording_transcripts_enabled) {
        auto control_transcript = std::make_shared<CaptionOutputControl<TranscriptOutputSettings>>(cur_settings.transcript_settings);
        transcript_recording_output.set_control(control_transcript);

        std::thread th2(transcript_writer_loop, control_transcript, false, cur_settings.transcript_settings);
        th2.detach();
    }
}

void SourceCaptioner::recording_stopped_event() {
    recording_output.clear();
    transcript_recording_output.clear();
}

void SourceCaptioner::set_text_source_text(const string &text_source_name, const string &caption_text) {
    if (std::get<0>(last_text_source_set) == caption_text && std::get<1>(last_text_source_set) == text_source_name) {
//        debug_log("ignore duplicate set_text_source_text");
        return;
    }

    ::set_text_source_text(text_source_name, caption_text);
    last_text_source_set = std::tuple<string, string>(caption_text, text_source_name);
}


SourceCaptioner::~SourceCaptioner() {
    stream_stopped_event();
    recording_stopped_event();
    stop_caption_stream(false);
}

template<class T>
void CaptionOutputControl<T>::stop_soon() {
    debug_log("CaptionOutputControl stop_soon()");
    stop = true;
    caption_queue.enqueue(CaptionOutput());
}

template<class T>
CaptionOutputControl<T>::~CaptionOutputControl() {
    debug_log("~CaptionOutputControl");
}

bool TranscriptOutputSettings::hasBaseSettings() const {
    if (!enabled || output_path.empty() || format.empty())
        return false;

    return true;
}
