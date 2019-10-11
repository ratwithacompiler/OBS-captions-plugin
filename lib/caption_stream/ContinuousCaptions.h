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


#ifndef CPPTESTING_CONTINUOUSCAPTIONS_H
#define CPPTESTING_CONTINUOUSCAPTIONS_H

#include <functional>
#include <CaptionStream.h>

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
            switchover_second_after_secs(connectSecondAfterSecs + switchoverSecondAfterSecs),
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

    void print(const char *line_prefix = "") {
        printf("%sContinuousCaptionStreamSettings\n", line_prefix);
        printf("%s  connect_second_after_secs: %d\n", line_prefix, connect_second_after_secs);
        printf("%s  switchover_second_after_secs: %d\n", line_prefix, switchover_second_after_secs);
        printf("%s  minimum_reconnect_interval_secs: %d\n", line_prefix, minimum_reconnect_interval_secs);

        stream_settings.print((string(line_prefix) + "  ").c_str());
//        printf("%s-----------\n", line_prefix);
    }

};

typedef std::function<void(const CaptionResult &caption_result, bool interrupted)> continuous_caption_text_callback;

/*
 Provides a continuous stream of caption messages for audio data, abstracting issues like reconnects after network errors and
 API limitation workarounds (Google Speech API v1 currently has a 5 minute maximum limit for a single streaming session).

 Minimizes impact of these regular disconnects by starting a second connection shortly before the first once
 is about to hit the limit and feeds both with the same audio for a bit before switching to the new one to avoid captioning gap.
 */
class ContinuousCaptions {
    std::shared_ptr<CaptionStream> current_stream;
    std::shared_ptr<CaptionStream> prepared_stream;

    std::chrono::steady_clock::time_point current_started_at;
    std::chrono::steady_clock::time_point prepared_started_at;

    ContinuousCaptionStreamSettings settings;

    bool interrupted;

    void on_caption_text_cb(const CaptionResult &caption_result);

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
