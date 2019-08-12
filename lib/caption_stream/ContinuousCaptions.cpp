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

#include <utility>

#include "ContinuousCaptions.h"
#include "log.h"

ContinuousCaptions::ContinuousCaptions(
        ContinuousCaptionStreamSettings settings
) :
        current_stream(nullptr),
        prepared_stream(nullptr),
        settings(settings),
        interrupted(false) {

}


bool ContinuousCaptions::queue_audio_data(const char *data, const uint data_size) {
    if (!data_size)
        return false;

    if (!current_stream) {
        debug_log("first time, no current stream, cycling");
        cycle_streams();
    }

    if (!current_stream) {
        error_log("WTF, NO UPSTREAM?????????????");
        return false;
    }

    double secs_since_start = std::chrono::duration_cast<std::chrono::duration<double >>(
            std::chrono::steady_clock::now() - current_started_at).count();

    if (current_stream->is_stopped()) {
        if (settings.minimum_reconnect_interval_secs && secs_since_start < settings.minimum_reconnect_interval_secs) {
//            debug_log("current stream dead, not reconnecting yet, too soon, %f %u",
//                      secs_since_start, settings.minimum_reconnect_interval_secs);
            return false;
        }
        debug_log("current stream dead, cycling, %f", secs_since_start);
        cycle_streams();

    }

    if (settings.switchover_second_after_secs && secs_since_start > settings.switchover_second_after_secs) {
        if (prepared_stream) {
            if (prepared_stream->is_stopped()) {
                debug_log("trying to switch to prepared stream but dead, recreating");
                clear_prepared();
                start_prepared();
            } else {
                debug_log("switching over to prepared stream, %f > %u", secs_since_start, settings.switchover_second_after_secs);
                cycle_streams();
            }
        }
    } else if (settings.connect_second_after_secs && secs_since_start > settings.connect_second_after_secs) {
        if (!prepared_stream || prepared_stream->is_stopped()) {
            debug_log("starting second stream %f", secs_since_start);
            start_prepared();
        } else {
//            debug_log("double queue");
            prepared_stream->queue_audio_data(data, data_size);
        }
    }
//    debug_log("queue");
    return current_stream->queue_audio_data(data, data_size);
}

void ContinuousCaptions::start_prepared() {
    debug_log("starting second prepared connection");
    clear_prepared();
    prepared_stream = std::make_shared<CaptionStream>(settings.stream_settings);
    if (!prepared_stream->start(prepared_stream)) {
        error_log("FAILED starting prepared connection");
    }

    prepared_started_at = std::chrono::steady_clock::now();
}

void ContinuousCaptions::clear_prepared() {
    if (!prepared_stream)
        return;

    debug_log("clearing prepared connection");
    prepared_stream->stop();
    prepared_stream = nullptr;
}

void ContinuousCaptions::cycle_streams() {
    debug_log("cycling streams");

    if (current_stream) {
        current_stream->stop();
        current_stream = nullptr;
    }

    if (prepared_stream && prepared_stream->is_stopped()) {
        prepared_stream->stop();
        prepared_stream = nullptr;
    }

    caption_text_callback cb = std::bind(&ContinuousCaptions::on_caption_text_cb, this, std::placeholders::_1);

    if (prepared_stream) {
        debug_log("cycling streams, using prepared connection");
        current_stream = prepared_stream;
        current_started_at = prepared_started_at;
        current_stream->on_caption_cb_handle.set(cb);
    } else {
        debug_log("cycling streams, creating new connection");
        current_stream = std::make_shared<CaptionStream>(settings.stream_settings);
        current_stream->on_caption_cb_handle.set(cb);
        if (!current_stream->start(current_stream))
            error_log("FAILED starting new connection");
        current_started_at = std::chrono::steady_clock::now();
    }
    prepared_stream = nullptr;
    interrupted = true;
}

void ContinuousCaptions::on_caption_text_cb(const string &data) {
//    debug_log("got caption data");
//    debug_log("got caption data %s", data.c_str());

    {
        std::lock_guard<recursive_mutex> lock(on_caption_cb_handle.mutex);
        if (on_caption_cb_handle.callback_fn) {
            on_caption_cb_handle.callback_fn(data, interrupted);
        }
    }

    interrupted = false;
}


ContinuousCaptions::~ContinuousCaptions() {
    debug_log("~ContinuousCaptions() decons");
    on_caption_cb_handle.clear();

    if (current_stream) {
        current_stream->stop();
    }

    if (prepared_stream) {
        prepared_stream->stop();
    }

}

