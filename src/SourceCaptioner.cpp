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


SourceCaptioner::SourceCaptioner(CaptionerSettings settings) :
        QObject(),
        settings(settings),
        last_caption_at(std::chrono::steady_clock::now()),
        last_caption_cleared(true) {

    QObject::connect(&timer, &QTimer::timeout, this, &SourceCaptioner::clear_output_timer_cb);
    timer.start(1000);

    info_log("SourceCaptioner, source '%s'", settings.caption_source_settings.caption_source_name.c_str());
    set_settings(settings);
}


void SourceCaptioner::clear_settings(bool send_signal) {
    {
        std::lock_guard<recursive_mutex> lock(settings_change_mutex);
        audio_capture_session = nullptr;
        caption_result_handler = nullptr;
        captioner = nullptr;
    }
    if (send_signal)
        not_not_captioning_status();
}

CaptionerSettings SourceCaptioner::get_settings() {
    std::lock_guard<recursive_mutex> lock(settings_change_mutex);
    return settings;
}

bool SourceCaptioner::set_settings(CaptionerSettings new_settings) {
    info_log("set_settingsset_settingsset_settings");
    clear_settings(false);
    {
        std::lock_guard<recursive_mutex> lock(settings_change_mutex);

        settings = new_settings;

        if (new_settings.caption_source_settings.caption_source_name.empty()) {
            warn_log("SourceCaptioner set_settings, empty source given.", new_settings.caption_source_settings.caption_source_name.c_str());
            clear_settings();
            return false;
        }

        OBSSource caption_source = obs_get_source_by_name(new_settings.caption_source_settings.caption_source_name.c_str());
        if (!caption_source) {
            warn_log("SourceCaptioner set_settings, no caption source with name: '%s'",
                     new_settings.caption_source_settings.caption_source_name.c_str());
            clear_settings();
            return false;
        }

        OBSSource mute_source;
        if (new_settings.caption_source_settings.mute_when == CAPTION_SOURCE_MUTE_TYPE_USE_OTHER_MUTE_SOURCE) {
            mute_source = obs_get_source_by_name(new_settings.caption_source_settings.mute_source_name.c_str());

            if (!mute_source) {
                warn_log("SourceCaptioner set_settings, no mute source with name: '%s'",
                         new_settings.caption_source_settings.mute_source_name.c_str());
                clear_settings();
                return false;
            }
        }

        auto caption_cb = std::bind(&SourceCaptioner::on_caption_text_callback, this, std::placeholders::_1, std::placeholders::_2);
        captioner = std::make_unique<ContinuousCaptions>(new_settings.stream_settings);
        captioner->on_caption_cb_handle.set(caption_cb, true);
        caption_result_handler = std::make_unique<CaptionResultHandler>(new_settings.format_settings);

        try {
            resample_info resample_to = {16000, AUDIO_FORMAT_16BIT, SPEAKERS_MONO};
            audio_chunk_data_cb audio_cb = std::bind(&SourceCaptioner::on_audio_data_callback, this,
                                                     std::placeholders::_1, std::placeholders::_2);

            auto audio_status_cb = std::bind(&SourceCaptioner::on_audio_capture_status_change_callback, this, std::placeholders::_1);

            audio_capture_session = std::make_unique<AudioCaptureSession>(caption_source, mute_source, audio_cb, audio_status_cb,
                                                                          resample_to,
//                                                                      MUTED_SOURCE_DISCARD_WHEN_MUTED);
                                                                          MUTED_SOURCE_REPLACE_WITH_ZERO);
        }
        catch (std::string err) {
            warn_log("couldn't create AudioCaptureSession, %s", err.c_str());
            clear_settings();
            return false;
        }
        catch (...) {
            warn_log("couldn't create AudioCaptureSession");
            clear_settings();
            return false;
        }

        info_log("starting captioning source '%s'", new_settings.caption_source_settings.caption_source_name.c_str());
        return true;
    }
}

void SourceCaptioner::on_audio_capture_status_change_callback(const audio_source_capture_status status) {
    info_log("capture status change %d ", status);
    emit audio_capture_status_changed(status);
}

void SourceCaptioner::on_audio_data_callback(const uint8_t *data, const size_t size) {
//    info_log("audio data");
    if (captioner) {
        captioner->queue_audio_data((char *) data, size);
    }
    audio_chunk_count++;

}

void SourceCaptioner::clear_output_timer_cb() {
//    info_log("clear timer checkkkkkkkkkkkkkkk");

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
    }

    output_caption_text(CaptionOutput(), true);
    emit caption_result_received(nullptr, false, true, "");
}

static void join_strings(const vector<string> &lines, char join_char, string &output) {
    for (const string &a_line: lines) {
//        info_log("a line: %s", a_line.c_str());

        if (!output.empty())
            output.push_back(join_char);

        output.append(a_line);
    }
}

void SourceCaptioner::store_result(shared_ptr<OutputCaptionResult> output_result, bool interrupted) {
    if (!output_result)
        return;

    if (interrupted) {
        if (held_nonfinal_caption_result) {
            results_history.push_back(held_nonfinal_caption_result);
            debug_log("interrupt, saving latest nonfinal result to history, %s",
                      held_nonfinal_caption_result->clean_caption_text.c_str());
        }
    }

    held_nonfinal_caption_result = nullptr;
    if (output_result->caption_result.final) {
        results_history.push_back(output_result);
        debug_log("final, adding to history: %s", output_result->clean_caption_text.c_str());
    } else {
        held_nonfinal_caption_result = output_result;
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
    shared_ptr<OutputCaptionResult> output_result;
    string output_caption_line;
    string recent_caption_text;
    {
        std::lock_guard<recursive_mutex> lock(settings_change_mutex);

        output_result = caption_result_handler->prepare_caption_output(caption_result, true, results_history);
        if (!output_result)
            return;

        store_result(output_result, interrupted);

        if (!output_result->output_lines.empty()) {
//            info_log("got lines %lu", output_result->output_lines.size());

            char join_char = settings.format_settings.caption_insert_newlines ? '\n' : ' ';
            join_strings(output_result->output_lines, join_char, output_caption_line);
        }

        if (output_caption_line == last_output_line || output_caption_line.empty()) {
//            debug_log("ignoring duplicate: '%s'", output_caption_line.c_str());
            return;
        }
        last_output_line = output_caption_line;

//        info_log("got caption '%s'", output_result->clean_caption_text.c_str());
//        info_log("got caption obj '%s'", output_result->caption_result.raw_message.c_str());
//        info_log("output line '%s'", output_caption_line.c_str());

        prepare_recent(recent_caption_text);
    }

    if (!output_caption_line.empty()) {
        this->output_caption_text(CaptionOutput(output_caption_line, output_result->caption_result.created_at));
    }

    emit caption_result_received(output_result, interrupted, false, recent_caption_text);
}

void SourceCaptioner::output_caption_text(const CaptionOutput &output, bool is_clearance) {

    bool sent_stream = false;
    {
        std::lock_guard<recursive_mutex> lock(caption_stream_output_mutex);
        if (caption_stream_output_control) {
            caption_stream_output_control->caption_queue.enqueue(output);
            sent_stream = true;
        }
    }

    bool sent_recording = false;
    {
        std::lock_guard<recursive_mutex> lock(caption_recording_output_mutex);
        if (caption_recording_output_control) {
            caption_recording_output_control->caption_queue.enqueue(output);
            sent_recording = true;
        }
    }

    debug_log("queuing caption line , stream: %d, recording: %d, '%s'",
              sent_stream, sent_recording, output.line.c_str());

    if (!is_clearance)
        caption_was_output();
}


void SourceCaptioner::not_not_captioning_status() {
    emit audio_capture_status_changed(-1);
}

void SourceCaptioner::caption_was_output() {
    this->last_caption_at = std::chrono::steady_clock::now();
    this->last_caption_cleared = false;
}


void SourceCaptioner::stream_started_event() {
    {
        std::lock_guard<recursive_mutex> lock(caption_stream_output_mutex);
        if (caption_stream_output_control) {
            caption_stream_output_control->stop_soon();
            caption_stream_output_control = nullptr;
        }

        caption_stream_output_control = new CaptionOutputControl();
        std::thread th(caption_output_writer_loop, caption_stream_output_control, true, true);
        th.detach();
    }
}

void SourceCaptioner::stream_stopped_event() {
    {
        std::lock_guard<recursive_mutex> lock(caption_stream_output_mutex);
        if (caption_stream_output_control) {
            caption_stream_output_control->stop_soon();
            caption_stream_output_control = nullptr;
        }
    }
}

void SourceCaptioner::recording_started_event() {
    {
        std::lock_guard<recursive_mutex> lock(caption_recording_output_mutex);
        if (caption_recording_output_control) {
            caption_recording_output_control->stop_soon();
            caption_recording_output_control = nullptr;
        }

        caption_recording_output_control = new CaptionOutputControl();
        std::thread th(caption_output_writer_loop, caption_recording_output_control, true, false);
        th.detach();
    }
}

void SourceCaptioner::recording_stopped_event() {
    {
        std::lock_guard<recursive_mutex> lock(caption_recording_output_mutex);
        if (caption_recording_output_control) {
            caption_recording_output_control->stop_soon();
            caption_recording_output_control = nullptr;
        }
    }
}


SourceCaptioner::~SourceCaptioner() {
    stream_stopped_event();
    recording_stopped_event();
    clear_settings(false);
}
