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

    QObject::connect(this, &SourceCaptioner::caption_text_line_received, this, &SourceCaptioner::send_caption_text, Qt::QueuedConnection);
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

    int active_delay = 0;
    obs_output_t *output = obs_frontend_get_streaming_output();
    if (output) {
//        info_log("built caption lines, sending: '%s'", output_caption_line.c_str());
        active_delay = obs_output_get_active_delay(output);
        obs_output_output_caption_text2(output, "", 0.01);
        obs_output_release(output);
    }

    emit caption_result_received(nullptr, false, true, active_delay, "");
}

void SourceCaptioner::send_caption_text(const string text, int send_in_secs) {
    if (send_in_secs) {
        auto call = [this, text, send_in_secs]() {
            info_log("SLOT sending lines, was delayed, waited %d  '%s'", send_in_secs, text.c_str());
            obs_output_t *output = obs_frontend_get_streaming_output();
            if (output) {
                // TODO: add must_match_output_delay bool param, check if delay is still the same if true
                // to avoid old ones firing after delay was turned off?

                this->caption_was_output();
                obs_output_output_caption_text2(output, text.c_str(), 0.01);
                obs_output_release(output);
            }
        };
        this->timer.singleShot(send_in_secs * 1000, this, call);
    } else {
        info_log("SLOT sending lines direct, sending,  '%s'", text.c_str());
        obs_output_t *output = obs_frontend_get_streaming_output();
        if (output) {
            this->caption_was_output();
            obs_output_output_caption_text2(output, text.c_str(), 0.01);
            obs_output_release(output);
        }
    }
}

void SourceCaptioner::on_caption_text_callback(const CaptionResult &caption_result, bool interrupted) {
    shared_ptr<OutputCaptionResult> output_result;
    string output_caption_line;
    string recent_captions;
    {
        std::lock_guard<recursive_mutex> lock(settings_change_mutex);

        output_result = caption_result_handler->prepare_caption_output(caption_result, true, results_history);
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
        if (caption_result.final) {
            results_history.push_back(output_result);
            debug_log("final, adding to history: %s", output_result->clean_caption_text.c_str());
        } else {
            held_nonfinal_caption_result = output_result;
        }

        if (!output_result->output_lines.empty()) {
//            info_log("got lines %lu", output_result->output_lines.size());
//
//             "\n".join(lines) or " ".join() depending on setting
            for (string &a_line: output_result->output_lines) {
//                info_log("a line: %s", a_line.c_str());
                if (!output_caption_line.empty())
                    if (settings.format_settings.caption_insert_newlines)
                        output_caption_line.push_back('\n');
                    else
                        output_caption_line.push_back(' ');

                output_caption_line.append(a_line);
            }
        }

        if (output_caption_line == last_output_line) {
//            debug_log("ignoring duplicate: '%s'", output_caption_line.c_str());
            return;
        }
        last_output_line = output_caption_line;

//        info_log("got caption '%s'", output_result->clean_caption_text.c_str());
//        info_log("got caption obj '%s'", output_result->caption_result.raw_message.c_str());
//        info_log("output line '%s'", output_caption_line.c_str());

        for (auto i = results_history.rbegin(); i != results_history.rend(); ++i) {
            if (!(*i))
                break;

            if (!(*i)->caption_result.final)
                break;

            if (recent_captions.size() + (*i)->clean_caption_text.size() >= MAX_HISTORY_VIEW_LENGTH)
                break;

            if ((*i)->clean_caption_text.empty())
                continue;

            if (recent_captions.empty()) {
                recent_captions.insert(0, 1, '.');
            } else {
                recent_captions.insert(0, ". ");
            }

            recent_captions.insert(0, (*i)->clean_caption_text);
        }

        if (held_nonfinal_caption_result) {
            if (!recent_captions.empty())
                recent_captions.push_back(' ');
            recent_captions.append("    >> ");
            recent_captions.append(held_nonfinal_caption_result->clean_caption_text);
        }
    }

    int active_delay_sec = 0;
    if (!output_caption_line.empty()) {
        active_delay_sec = this->output_caption_text(output_caption_line);
    }

    emit caption_result_received(output_result, interrupted, false, active_delay_sec, recent_captions);
}

int SourceCaptioner::output_caption_text(const string &line) {
    int active_delay_sec = 0;

    obs_output_t *output = obs_frontend_get_streaming_output();
    if (output) {
        active_delay_sec = obs_output_get_active_delay(output);
        if (active_delay_sec) {
            debug_log("queueing caption lines, preparing delay: %d,  '%s'",
                      active_delay_sec, line.c_str());

            string text(line);
            emit this->caption_text_line_received(text, active_delay_sec);

        } else {
            debug_log("sending caption lines, sending direct now: '%s'", line.c_str());
            this->caption_was_output();
            obs_output_output_caption_text2(output, line.c_str(), 0.01);
        }
    } else {
//            info_log("built caption lines, no output, not sending, not live?: '%s'", output_caption_line.c_str());
    }
    obs_output_release(output);

    return active_delay_sec;
}

SourceCaptioner::~SourceCaptioner() {
    clear_settings(false);
}

void SourceCaptioner::not_not_captioning_status() {
    emit audio_capture_status_changed(-1);
}

void SourceCaptioner::caption_was_output() {
    this->last_caption_at = std::chrono::steady_clock::now();
    this->last_caption_cleared = false;
}

