//
// Created by Rat on 2019-06-10.
//

#ifndef CPPTESTING_CONTINUOUSCAPTIONS_H
#define CPPTESTING_CONTINUOUSCAPTIONS_H


#include "CaptionStream.h"

struct ContinuousCaptionStreamSettings {
    uint connect_second_after_secs;
    uint switchover_second_after_secs;

    uint minimum_reconnect_interval_secs;

    CaptionStreamSettings stream_settings;

    ContinuousCaptionStreamSettings(
            uint connectSecondAfterSecs,
            uint switchoverSecondAfterSecs,
            uint minimumReconnectIntervalSecs,
            CaptionStreamSettings streamSettings
    ) :
            connect_second_after_secs(connectSecondAfterSecs),
            switchover_second_after_secs(switchoverSecondAfterSecs),
            minimum_reconnect_interval_secs(minimumReconnectIntervalSecs),
            stream_settings(streamSettings) {}

    bool operator==(const ContinuousCaptionStreamSettings &rhs) const {
        return connect_second_after_secs == rhs.connect_second_after_secs &&
               switchover_second_after_secs == rhs.switchover_second_after_secs &&
               minimum_reconnect_interval_secs == rhs.minimum_reconnect_interval_secs &&
               stream_settings == rhs.stream_settings;
    }

    bool operator!=(const ContinuousCaptionStreamSettings &rhs) const {
        return !(rhs == *this);
    }

    void print() {
        printf("ContinuousCaptionStreamSettings\n");
        printf("    connect_second_after_secs: %d\n", connect_second_after_secs);
        printf("    switchover_second_after_secs: %d\n", switchover_second_after_secs);
        printf("    minimum_reconnect_interval_secs: %d\n", minimum_reconnect_interval_secs);

        stream_settings.print();
        printf("-----------");
    }

};

typedef std::function<void(const string &data, bool interrupted)> continuous_caption_text_callback;

class ContinuousCaptions {
    std::shared_ptr<CaptionStream> current_stream;
    std::shared_ptr<CaptionStream> prepared_stream;

    std::chrono::steady_clock::time_point current_started_at;
    std::chrono::steady_clock::time_point prepared_started_at;

    ContinuousCaptionStreamSettings settings;

    bool interrupted;

    void on_caption_text_cb(const string &data);

    void start_prepared();

    void clear_prepared();

    void cycle_streams();

public:
    ThreadsaferCallback<continuous_caption_text_callback> on_caption_cb_handle;

    ContinuousCaptions(
            ContinuousCaptionStreamSettings settings
    );

    bool queue_audio_data(const char *data, const uint data_size);


    ~ContinuousCaptions();
};


#endif //CPPTESTING_CONTINUOUSCAPTIONS_H
