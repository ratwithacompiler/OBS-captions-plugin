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

#ifndef CPPTESTING_CAPTIONSTREAM_H
#define CPPTESTING_CAPTIONSTREAM_H

#include <functional>
#include "TcpConnection.h"
#include <iostream>
#include <thread>
#include <string>
#include <queue>
#include <chrono>
#include <mutex>
#include <atomic>
#include <cameron314/blockingconcurrentqueue.h>
#include <ThreadsaferCallback.h>
#include <CaptionResult.h>
#include <sstream>

typedef unsigned int uint;
using namespace std;

typedef std::function<void(const CaptionResult &caption_result)> caption_text_callback;

struct CaptionStreamSettings {
    uint connect_timeout_ms;
    uint send_timeout_ms;
    uint recv_timeout_ms;

    uint max_queue_depth;
    uint download_thread_start_delay_ms;

    string language;
    int profanity_filter;
    string api_key;

    CaptionStreamSettings(
            uint connect_timeout_ms,
            uint send_timeout_ms,
            uint recv_timeout_ms,

            uint max_queue_depth,
            uint download_thread_start_delay_ms,
            const string &language,
            int profanity_filter,
            const string &api_key
    ) :
            connect_timeout_ms(connect_timeout_ms),
            send_timeout_ms(send_timeout_ms),
            recv_timeout_ms(recv_timeout_ms),

            max_queue_depth(max_queue_depth),
            download_thread_start_delay_ms(download_thread_start_delay_ms),
            language(language),
            profanity_filter(profanity_filter),
            api_key(api_key) {}

    bool operator==(const CaptionStreamSettings &rhs) const {
        return connect_timeout_ms == rhs.connect_timeout_ms &&
               send_timeout_ms == rhs.send_timeout_ms &&
               recv_timeout_ms == rhs.recv_timeout_ms &&
               max_queue_depth == rhs.max_queue_depth &&
               download_thread_start_delay_ms == rhs.download_thread_start_delay_ms &&
               language == rhs.language &&
               profanity_filter == rhs.profanity_filter &&
               api_key == rhs.api_key;
    }

    bool operator!=(const CaptionStreamSettings &rhs) const {
        return !(rhs == *this);
    }

//    CaptionStreamSettings() {};


    string describe(const string &sep = "\n", const string &line_prefix = "") const {
        std::ostringstream os;
        os << line_prefix << "CaptionStreamSettings";
        os << sep << line_prefix << "  connect_timeout_ms: " << connect_timeout_ms;
        os << sep << line_prefix << "  send_timeout_ms: " << send_timeout_ms;
        os << sep << line_prefix << "  recv_timeout_ms: " << recv_timeout_ms;
        os << sep << line_prefix << "  max_queue_depth: " << max_queue_depth;
        os << sep << line_prefix << "  download_thread_start_delay_ms: " << download_thread_start_delay_ms;
        return os.str();
    }

    void print(const char *line_prefix = "") const {
        printf("%s\n", describe("\n", line_prefix).c_str());
    }
};


class CaptionStream {
    TcpConnection upstream;
    TcpConnection downstream;

    CaptionStreamSettings settings;
    string session_pair;

    std::thread *upstream_thread = nullptr;
    std::thread *downstream_thread = nullptr;

    moodycamel::BlockingConcurrentQueue<string *> audio_queue;

    std::atomic<bool> started{false};
    std::atomic<bool> stopped{false};

    string *dequeue_audio_data(const std::int64_t timeout_us);

    void upstream_run(std::shared_ptr<CaptionStream> self);

    void _upstream_run(std::shared_ptr<CaptionStream> self);


    void downstream_run(std::shared_ptr<CaptionStream> self);

    void _downstream_run();

public:
    ThreadsaferCallback<caption_text_callback> on_caption_cb_handle;

    CaptionStream(
            CaptionStreamSettings settings
    );

    bool start(std::shared_ptr<CaptionStream> self);

    void stop();

    bool is_connected();

    bool is_started();

    bool is_stopped();

    bool queue_audio_data(const char *data, const uint data_size);

    ~CaptionStream();
};

#endif //CPPTESTING_CAPTIONSTREAM_H
