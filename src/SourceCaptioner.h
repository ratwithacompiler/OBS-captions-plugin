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


#include <ContinuousCaptions.h>
#include "AudioCaptureSession.h"
#include "CaptionResultHandler.h"
#include "caption_output_writer.h"

#include <QObject>
#include <QTimer>

typedef unsigned int uint;
using namespace std;

Q_DECLARE_METATYPE(std::string)

Q_DECLARE_METATYPE(shared_ptr<OutputCaptionResult>)

Q_DECLARE_METATYPE(CaptionResult)

#define MAX_HISTORY_VIEW_LENGTH 2000

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
    bool streaming_output_enabled;
    bool recording_output_enabled;
    CaptionSourceSettings caption_source_settings;

    CaptionFormatSettings format_settings;
    ContinuousCaptionStreamSettings stream_settings;

    CaptionerSettings(
            bool enabled,
            bool streaming_output_enabled,
            bool recording_output_enabled,
            const CaptionSourceSettings caption_source_settings,
            const CaptionFormatSettings format_settings,
            const ContinuousCaptionStreamSettings stream_settings
    ) :
            enabled(enabled),
            streaming_output_enabled(streaming_output_enabled),
            recording_output_enabled(recording_output_enabled),
            caption_source_settings(caption_source_settings),
            format_settings(format_settings),
            stream_settings(stream_settings) {}

    bool operator==(const CaptionerSettings &rhs) const {
        return enabled == rhs.enabled &&
               streaming_output_enabled == rhs.streaming_output_enabled &&
               recording_output_enabled == rhs.recording_output_enabled &&
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
        printf("    streaming_output_enabled: %d\n", streaming_output_enabled);
        printf("    recording_output_enabled: %d\n", recording_output_enabled);
        printf("    string: %s\n", caption_source_settings.caption_source_name.c_str());
        stream_settings.print();
        format_settings.print();
        printf("-----------");
    }
};

struct OutputWriter {
    std::recursive_mutex control_change_mutex;
    std::shared_ptr<CaptionOutputControl> control;

    void clear() {
        std::lock_guard<recursive_mutex> lock(control_change_mutex);
        if (control) {
            control->stop_soon();
            control = nullptr;
        }
    }

    void set_control(const std::shared_ptr<CaptionOutputControl> &set_control) {
        std::lock_guard<recursive_mutex> lock(control_change_mutex);
        clear();
        this->control = set_control;
    }

    bool enqueue(const CaptionOutput &output) {
        std::lock_guard<recursive_mutex> lock(control_change_mutex);
        if (control) {
            control->caption_queue.enqueue(output);
            return true;
        }
        return false;
    }
};

class SourceCaptioner : public QObject {
Q_OBJECT

    std::unique_ptr<AudioCaptureSession> audio_capture_session;
    std::unique_ptr<ContinuousCaptions> captioner;
    uint audio_chunk_count = 0;

    CaptionerSettings settings;
    std::unique_ptr<CaptionResultHandler> caption_result_handler;
    std::recursive_mutex settings_change_mutex;

    std::chrono::steady_clock::time_point last_caption_at;
    bool last_caption_cleared;
    QTimer timer;

    std::vector<std::shared_ptr<OutputCaptionResult>> results_history; // final ones + last ones before interruptions
    std::shared_ptr<OutputCaptionResult> held_nonfinal_caption_result;

    OutputWriter streaming_output;
    OutputWriter recording_output;

    void caption_was_output();

    void output_caption_text(
            const CaptionOutput &output,
            bool to_stream,
            bool to_recoding,
            bool is_clearance = false
    );

    void store_result(shared_ptr<OutputCaptionResult> output_result, bool interrupted);

    void prepare_recent(string &recent_captions_output);

private slots:

    void clear_output_timer_cb();

    void process_caption_result(const CaptionResult, bool interrupted);

//    void send_caption_text(const string text, int send_in_secs);

signals:

    void caption_text_line_received(string caption_text, int delay_sec);

    void received_caption_result(const CaptionResult, bool interrupted);

    void caption_result_received(
            shared_ptr<OutputCaptionResult> caption,
            bool interrupted,
            bool cleared,
            string recent_caption_text);

    void audio_capture_status_changed(const int new_status);

public:

    SourceCaptioner(CaptionerSettings settings);

    ~SourceCaptioner();

    void clear_settings(bool send_signal = true);

    bool set_settings(CaptionerSettings settings);

    void on_audio_data_callback(const uint8_t *data, const size_t size);

    void on_audio_capture_status_change_callback(const audio_source_capture_status status);

    void on_caption_text_callback(const CaptionResult &caption_result, bool interrupted);

    void not_not_captioning_status();

    void stream_started_event();

    void stream_stopped_event();

    void recording_started_event();

    void recording_stopped_event();
};


#endif //OBS_STUDIO_SOURCECAPTIONER_H
