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

    void print(const char *line_prefix = "") {
        printf("%sCaptionSourceSettings\n", line_prefix);
        printf("%s  caption_source_name: %s\n", line_prefix, caption_source_name.c_str());
        printf("%s  mute_when: %d\n", line_prefix, mute_when);
        printf("%s  mute_source_name: %s\n", line_prefix, mute_source_name.c_str());
    }
};

struct TextOutputSettings {
    bool enabled;
    string text_source_name;
    uint line_length;
    uint line_count;
    CapitalizationType capitalization;
//    bool insert_newlines;

    void print(const char *line_prefix = "") {
        printf("%sTextOutputSettings\n", line_prefix);
        printf("%s  enabled: %d\n", line_prefix, enabled);
        printf("%s  text_source_name: %s\n", line_prefix, text_source_name.c_str());
        printf("%s  line_length: %d\n", line_prefix, line_length);
        printf("%s  line_count: %d\n", line_prefix, line_count);
        printf("%s  capitalization: %d\n", line_prefix, capitalization);
//        printf("%s  insert_newlines: %d\n", line_prefix, insert_newlines);
    }

    bool operator==(const TextOutputSettings &rhs) const {
        return enabled == rhs.enabled &&
               text_source_name == rhs.text_source_name &&
               line_length == rhs.line_length &&
               line_count == rhs.line_count &&
               capitalization == rhs.capitalization;
//               insert_newlines == rhs.insert_newlines;
    }

    bool operator!=(const TextOutputSettings &rhs) const {
        return !(rhs == *this);
    }
};

struct SceneCollectionSettings {
    CaptionSourceSettings caption_source_settings;
    TextOutputSettings text_output_settings;

    bool operator==(const SceneCollectionSettings &rhs) const {
        return caption_source_settings == rhs.caption_source_settings &&
               text_output_settings == rhs.text_output_settings;
    }

    bool operator!=(const SceneCollectionSettings &rhs) const {
        return !(rhs == *this);
    }
};

struct TranscriptOutputSettings {
    bool enabled;
    string output_path;
    string format;

    string recording_filename_type;
    string recording_filename_custom;
    string recording_filename_exists;

    string streaming_filename_type;
    string streaming_filename_custom;
    string streaming_filename_exists;

    uint srt_target_duration_secs;
    uint srt_target_line_length;

    bool streaming_transcripts_enabled;
    bool recording_transcripts_enabled;

    TranscriptOutputSettings(bool enabled, const string &outputPath, const string &format, const string &recordingFilenameChoice,
                             const string &recordingFilenameCustom, const string &recordingFilenameExists,
                             const string &streamingFilenameChoice, const string &streamingFilenameCustom,
                             const string &streamingFilenameExists, uint srtTargetDurationSecs,
                             uint srtTargetLineLength, bool streamingTranscriptsEnabled, bool recordingTranscriptsEnabled) :
            enabled(enabled), output_path(outputPath), format(format), recording_filename_type(recordingFilenameChoice),
            recording_filename_custom(recordingFilenameCustom), recording_filename_exists(recordingFilenameExists),
            streaming_filename_type(streamingFilenameChoice), streaming_filename_custom(streamingFilenameCustom),
            streaming_filename_exists(streamingFilenameExists), srt_target_duration_secs(srtTargetDurationSecs),
            srt_target_line_length(srtTargetLineLength), streaming_transcripts_enabled(streamingTranscriptsEnabled),
            recording_transcripts_enabled(recordingTranscriptsEnabled) {}

    bool operator==(const TranscriptOutputSettings &rhs) const {
        return enabled == rhs.enabled &&
               output_path == rhs.output_path &&
               format == rhs.format &&
               recording_filename_type == rhs.recording_filename_type &&
               recording_filename_custom == rhs.recording_filename_custom &&
               recording_filename_exists == rhs.recording_filename_exists &&
               streaming_filename_type == rhs.streaming_filename_type &&
               streaming_filename_custom == rhs.streaming_filename_custom &&
               streaming_filename_exists == rhs.streaming_filename_exists &&
               srt_target_duration_secs == rhs.srt_target_duration_secs &&
               srt_target_line_length == rhs.srt_target_line_length &&
               streaming_transcripts_enabled == rhs.streaming_transcripts_enabled &&
               recording_transcripts_enabled == rhs.recording_transcripts_enabled;
    }

    bool operator!=(const TranscriptOutputSettings &rhs) const {
        return !(rhs == *this);
    }

    bool hasBaseSettings() const;

    void print(const char *line_prefix = "") {
        printf("%sTranscriptSettings\n", line_prefix);
        printf("%s  enabled: %d\n", line_prefix, enabled);
        printf("%s  output_path: %s\n", line_prefix, output_path.c_str());
        printf("%s  format: %s\n", line_prefix, format.c_str());

        printf("%s  recording_filename_type: %s\n", line_prefix, recording_filename_type.c_str());
        printf("%s  recording_filename_custom: %s\n", line_prefix, recording_filename_custom.c_str());
        printf("%s  recording_filename_exists: %s\n", line_prefix, recording_filename_exists.c_str());

        printf("%s  streaming_filename_type: %s\n", line_prefix, streaming_filename_type.c_str());
        printf("%s  streaming_filename_custom: %s\n", line_prefix, streaming_filename_custom.c_str());
        printf("%s  streaming_filename_exists: %s\n", line_prefix, streaming_filename_exists.c_str());

        printf("%s  srt_target_duration_secs: %d\n", line_prefix, srt_target_duration_secs);
        printf("%s  srt_target_line_length: %d\n", line_prefix, srt_target_line_length);
        printf("%s  streaming_transcripts_enabled: %d\n", line_prefix, streaming_transcripts_enabled);
        printf("%s  recording_transcripts_enabled: %d\n", line_prefix, recording_transcripts_enabled);
    }
};

struct SourceCaptionerSettings {
    bool streaming_output_enabled;
    bool recording_output_enabled;

    TranscriptOutputSettings transcript_settings;

//    std::map<string, SceneCollectionSettings> scene_collection_settings_map;
    SceneCollectionSettings scene_collection_settings;

    CaptionFormatSettings format_settings;
    ContinuousCaptionStreamSettings stream_settings;

    SourceCaptionerSettings();

    SourceCaptionerSettings(
            bool streaming_output_enabled,
            bool recording_output_enabled,

            const TranscriptOutputSettings &transcript_settings,
            const SceneCollectionSettings &scene_collection_settings,
            const CaptionFormatSettings &format_settings,
            const ContinuousCaptionStreamSettings &stream_settings
    ) :
            streaming_output_enabled(streaming_output_enabled),
            recording_output_enabled(recording_output_enabled),

            transcript_settings(transcript_settings),
            scene_collection_settings(scene_collection_settings),
            format_settings(format_settings),
            stream_settings(stream_settings) {}

    bool operator==(const SourceCaptionerSettings &rhs) const {
        return streaming_output_enabled == rhs.streaming_output_enabled &&
               recording_output_enabled == rhs.recording_output_enabled &&

               transcript_settings == rhs.transcript_settings &&
               scene_collection_settings == rhs.scene_collection_settings &&
               format_settings == rhs.format_settings &&
               stream_settings == rhs.stream_settings;
//               scene_collection_settings_map == rhs.scene_collection_settings_map &&
    }


    bool operator!=(const SourceCaptionerSettings &rhs) const {
        return !(rhs == *this);
    }

    void print(const char *line_prefix = "") {
        printf("%sSourceCaptionerSettings\n", line_prefix);
        printf("%s  streaming_output_enabled: %d\n", line_prefix, streaming_output_enabled);
        printf("%s  recording_output_enabled: %d\n", line_prefix, recording_output_enabled);
        printf("%s  Scene Collection Settings:\n", line_prefix);
        scene_collection_settings.caption_source_settings.print((string(line_prefix) + "    ").c_str());
        scene_collection_settings.text_output_settings.print((string(line_prefix) + "    ").c_str());

//        printf("%s  Scene Collection Settings: %lu\n", line_prefix, scene_collection_settings_map.size());
//        for (auto it = scene_collection_settings_map.begin(); it != scene_collection_settings_map.end(); ++it) {
//            printf("%s   Collection: %s, \n", line_prefix, it->first.c_str());
//            it->second.caption_source_settings.print((string(line_prefix) + "    ").c_str());
//            it->second.text_output_settings.print((string(line_prefix) + "    ").c_str());
//        }

        stream_settings.print((string(line_prefix) + "  ").c_str());
        format_settings.print((string(line_prefix) + "  ").c_str());
        transcript_settings.print((string(line_prefix) + "  ").c_str());
    }

    const SceneCollectionSettings &get_scene_collection_settings(const string &scene_collection_name) const {
        return scene_collection_settings;
    }

    void update_setting(const string &scene_collection_name, const SceneCollectionSettings &new_settings) {
        scene_collection_settings = new_settings;
    }

//    const SceneCollectionSettings *get_scene_collection_settings(const string &scene_collection_name) const {
//       //TO[do]: insert default if it doesn't exist
//        auto it = scene_collection_settings_map.find(scene_collection_name);
//        if (it != scene_collection_settings_map.end())
//            return &(it->second);
//
//        return nullptr;
//    }
//
//    void update_setting(const string &scene_collection_name, const SceneCollectionSettings &new_settings) {
//        scene_collection_settings_map[scene_collection_name] = new_settings;
//    }
};

struct CaptionOutput {
    shared_ptr<OutputCaptionResult> output_result;
    bool is_clearance;

    CaptionOutput(shared_ptr<OutputCaptionResult> output_result, bool is_clearance) :
            output_result(output_result),
            is_clearance(is_clearance) {};

    CaptionOutput() : is_clearance(false) {}
};

template<typename T>
struct CaptionOutputControl {
    moodycamel::BlockingConcurrentQueue<CaptionOutput> caption_queue;
    volatile bool stop = false;
    T arg;

    CaptionOutputControl(T arg) : arg(arg) {}

    void stop_soon();

    ~CaptionOutputControl();
};

template<typename T>
struct OutputWriter {
    std::recursive_mutex control_change_mutex;
    std::shared_ptr<CaptionOutputControl<T>> control;

    void clear() {
        std::lock_guard<recursive_mutex> lock(control_change_mutex);
        if (control) {
            control->stop_soon();
            control = nullptr;
        }
    }

    void set_control(const std::shared_ptr<CaptionOutputControl<T>> &set_control) {
        std::lock_guard<recursive_mutex> lock(control_change_mutex);
        clear();
        this->control = set_control;
    }

    bool enqueue(const CaptionOutput &output) {
        std::lock_guard<recursive_mutex> lock(control_change_mutex);
        if (control && !control->stop) {
            control->caption_queue.enqueue(output);
            return true;
        }
        return false;
    }
};

enum SourceCaptionerStatusEvent {
    SOURCE_CAPTIONER_STATUS_EVENT_STOPPED,

    SOURCE_CAPTIONER_STATUS_EVENT_STARTED_OK,
    SOURCE_CAPTIONER_STATUS_EVENT_STARTED_ERROR,

    SOURCE_CAPTIONER_STATUS_EVENT_NEW_SETTINGS_STOPPED,

    SOURCE_CAPTIONER_STATUS_EVENT_AUDIO_CAPTURE_STATUS_CHANGE,
};

struct SourceCaptionerStatus {
    // TODO: add starting error msg string
    SourceCaptionerStatusEvent event_type;

    bool settings_changed;
    bool stream_settings_changed;
    SourceCaptionerSettings settings;
    string scene_collection_name;

    audio_source_capture_status audio_capture_status;

    bool active;

    SourceCaptionerStatus(SourceCaptionerStatusEvent eventType, bool settingsChanged, bool streamSettingsChanged,
                          const SourceCaptionerSettings &settings, string scene_collection_name,
                          audio_source_capture_status audioCaptureStatus, bool active)
            : event_type(eventType), settings_changed(settingsChanged), stream_settings_changed(streamSettingsChanged), settings(settings),
              scene_collection_name(scene_collection_name), audio_capture_status(audioCaptureStatus), active(active) {}
};

Q_DECLARE_METATYPE(std::shared_ptr<SourceCaptionerStatus>)

class SourceCaptioner : public QObject {
Q_OBJECT

    std::unique_ptr<AudioCaptureSession> audio_capture_session;
    std::unique_ptr<ContinuousCaptions> continuous_captions;
    uint audio_chunk_count = 0;

    SourceCaptionerSettings settings;
    string selected_scene_collection_name;

    std::unique_ptr<CaptionResultHandler> caption_result_handler;
    std::recursive_mutex settings_change_mutex;

    std::chrono::steady_clock::time_point last_caption_at;
    bool last_caption_cleared;
    QTimer timer;

    std::vector<std::shared_ptr<OutputCaptionResult>> results_history; // final ones + last ones before interruptions
    std::shared_ptr<OutputCaptionResult> held_nonfinal_caption_result;

    OutputWriter<int> streaming_output;
    OutputWriter<int> recording_output;

    OutputWriter<TranscriptOutputSettings> transcript_streaming_output;
    OutputWriter<TranscriptOutputSettings> transcript_recording_output;

    std::tuple<string, string> last_text_source_set;

    int audio_capture_id = 0;

    string last_caption_text;
    bool last_caption_final = false;

    void caption_was_output();

    void output_caption_writers(
            const CaptionOutput &output,
            bool to_stream,
            bool to_recoding,
            bool to_transcript_streaming,
            bool to_transcript_recording,
            bool is_clearance
    );

    void store_result(shared_ptr<OutputCaptionResult> output_result);

    void prepare_recent(string &recent_captions_output);

    void on_audio_data_callback(const int id, const uint8_t *data, const size_t size);

    void on_audio_capture_status_change_callback(const int id, const audio_source_capture_status status);

    void on_caption_text_callback(const CaptionResult &caption_result, bool interrupted);

    bool _start_caption_stream(bool restart_stream);

    void process_caption_result(const CaptionResult, bool interrupted);

    void process_audio_capture_status_change(const int id, const int new_status);

    void set_text_source_text(const string &text_source_name, const string &caption_text);

private slots:

    void clear_output_timer_cb();

//    void send_caption_text(const string text, int send_in_secs);

signals:

    void received_caption_result(const CaptionResult, bool interrupted);

    void caption_result_received(
            shared_ptr<OutputCaptionResult> caption,
            bool cleared,
            string recent_caption_text);

    void audio_capture_status_changed(const int id, const int new_status);

    void source_capture_status_changed(shared_ptr<SourceCaptionerStatus> status);

public:

    SourceCaptioner(const SourceCaptionerSettings &settings, const string &scene_collection_name, bool start);

    ~SourceCaptioner();

    bool set_settings(const SourceCaptionerSettings &new_settings, const string &scene_collection_name);

    bool start_caption_stream(const SourceCaptionerSettings &new_settings, const string &scene_collection_name);

    void stop_caption_stream(bool send_signal = true);


    void stream_started_event();

    void stream_stopped_event();

    void recording_started_event();

    void recording_stopped_event();

};


#endif //OBS_STUDIO_SOURCECAPTIONER_H
