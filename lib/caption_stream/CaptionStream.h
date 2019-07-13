#ifndef CPPTESTING_CAPTIONSTREAM_H
#define CPPTESTING_CAPTIONSTREAM_H


#include "TcpConnection.h"
#include <iostream>
#include <thread>
#include <string>
#include <queue>
#include <chrono>
#include <mutex>
#include "thirdparty/cameron314/blockingconcurrentqueue.h"
#include "log.h"

#include "ThreadsaferCallback.h"

typedef unsigned int uint;
using namespace std;

typedef std::function<void(const string &data)> caption_text_callback;

struct CaptionStreamSettings {
    uint connect_timeout_ms;
    uint send_timeout_ms;
    uint recv_timeout_ms;

    uint max_queue_depth;
    uint download_thread_start_delay_ms;

    CaptionStreamSettings(
            uint connect_timeout_ms,
            uint send_timeout_ms,
            uint recv_timeout_ms,

            uint max_queue_depth,
            uint download_thread_start_delay_ms
    ) :
            connect_timeout_ms(connect_timeout_ms),
            send_timeout_ms(send_timeout_ms),
            recv_timeout_ms(recv_timeout_ms),

            max_queue_depth(max_queue_depth),
            download_thread_start_delay_ms(download_thread_start_delay_ms) {}

    bool operator==(const CaptionStreamSettings &rhs) const {
        return connect_timeout_ms == rhs.connect_timeout_ms &&
               send_timeout_ms == rhs.send_timeout_ms &&
               recv_timeout_ms == rhs.recv_timeout_ms &&
               max_queue_depth == rhs.max_queue_depth &&
               download_thread_start_delay_ms == rhs.download_thread_start_delay_ms;
    }

    bool operator!=(const CaptionStreamSettings &rhs) const {
        return !(rhs == *this);
    }

//    CaptionStreamSettings() {};


    void print() {
        printf("CaptionStreamSettings\n");
        printf("    connect_timeout_ms: %d\n", connect_timeout_ms);
        printf("    send_timeout_ms: %d\n", send_timeout_ms);
        printf("    recv_timeout_ms: %d\n", recv_timeout_ms);

        printf("    max_queue_depth: %d\n", max_queue_depth);
        printf("    download_thread_start_delay_ms: %d\n", download_thread_start_delay_ms);

        printf("-----------");
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

    bool started = false;
    bool stopped = false;

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
