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



#ifndef OBS_STUDIO_SOURCECAPTIONER_H
#define OBS_STUDIO_SOURCECAPTIONER_H


#include "AudioCaptureSession.h"
#include "ContinuousCaptions.h"
#include "CaptionResultHandler.h"

#include <QObject>
#include <QTimer>

typedef unsigned int uint;
using namespace std;

Q_DECLARE_METATYPE(std::string)

Q_DECLARE_METATYPE(shared_ptr<CaptionResult>)

enum CaptionSourceMuteType {
    CAPTION_SOURCE_MUTE_TYPE_FROM_OWN_SOURCE,
    CAPTION_SOURCE_MUTE_TYPE_ALWAYS_CAPTION,
    CAPTION_SOURCE_MUTE_TYPE_USE_OTHER_MUTE_SOURCE,
};

struct CaptionSourceSettings {
    string caption_source_name;
    CaptionSourceMuteType mute_when;
    string mute_source_name;

    bool operator==(const CaptionSourceSettings &rhs) const {
        return caption_source_name == rhs.caption_source_name &&
               mute_when == rhs.mute_when &&
               mute_source_name == rhs.mute_source_name;
    }

    bool operator!=(const CaptionSourceSettings &rhs) const {
        return !(rhs == *this);
    }
};

struct CaptionerSettings {
    bool enabled;
    CaptionSourceSettings caption_source_settings;

    CaptionFormatSettings format_settings;
    ContinuousCaptionStreamSettings stream_settings;

    CaptionerSettings(
            bool enabled,
            const CaptionSourceSettings caption_source_settings,
            const CaptionFormatSettings format_settings,
            const ContinuousCaptionStreamSettings stream_settings
    ) :
            enabled(enabled),
            caption_source_settings(caption_source_settings),
            format_settings(format_settings),
            stream_settings(stream_settings) {}

    bool operator==(const CaptionerSettings &rhs) const {
        return enabled == rhs.enabled &&
               caption_source_settings == rhs.caption_source_settings &&
               format_settings == rhs.format_settings &&
               stream_settings == rhs.stream_settings;
    }


    bool operator!=(const CaptionerSettings &rhs) const {
        return !(rhs == *this);
    }

    void print() {
        printf("CaptionerSettings\n");
        printf("    enabled: %d\n", enabled);
        printf("    string: %s\n", caption_source_settings.caption_source_name.c_str());
        stream_settings.print();
        format_settings.print();
        printf("-----------");
    }
};

class SourceCaptioner : public QObject {
Q_OBJECT

    std::unique_ptr<AudioCaptureSession> audio_capture_session;
    std::unique_ptr<ContinuousCaptions> captioner;
    uint audio_chunk_count = 0;

    CaptionerSettings settings;
    std::unique_ptr<CaptionResultHandler> caption_parser;
    std::recursive_mutex settings_change_mutex;

    std::chrono::steady_clock::time_point last_caption_at;
    bool last_caption_cleared;
    QTimer timer;

    void caption_was_output();

    int output_caption_text(const string &line);

private slots:

    void clear_output_timer_cb();

    void send_caption_text(const string text, int send_in_secs);

signals:

    void caption_text_line_received(string caption_text, int delay_sec);

    void caption_result_received(shared_ptr<CaptionResult> caption, bool interrupted, bool cleared, int active_delay_sec);

    void audio_capture_status_changed(const int new_status);

public:

    SourceCaptioner(CaptionerSettings settings);

    ~SourceCaptioner();

    void clear_settings(bool send_signal = true);

    bool set_settings(CaptionerSettings settings);

    CaptionerSettings get_settings();

    void on_audio_data_callback(const uint8_t *data, const size_t size);

    void on_audio_capture_status_change_callback(const audio_source_capture_status status);

    void on_caption_text_callback(const string &caption_obj, bool interrupted);

    void not_not_captioning_status();


};


#endif //OBS_STUDIO_SOURCECAPTIONER_H
