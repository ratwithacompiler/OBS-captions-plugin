//
// Created by Rat on 05.11.19.
//

#ifndef OBS_GOOGLE_CAPTION_PLUGIN_CAPTION_TRANSCRIPT_WRITER_H
#define OBS_GOOGLE_CAPTION_PLUGIN_CAPTION_TRANSCRIPT_WRITER_H


#include "log.c"
#include <QDir>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <sstream>
#include <deque>
#include "SourceCaptioner.h"

using ResultQueue = std::deque<std::shared_ptr<OutputCaptionResult>>;

const string NEWLINE_STR = "\r\n";

static void split_into_non_wrapped_lines(vector<string> &output_lines, const string &text, const uint max_line_length) {
    istringstream stream(text);
    string word;
    string line;
    while (getline(stream, word, ' ')) {
        if (word.empty())
            continue;

        int new_len = line.size() + (line.empty() ? 0 : 1) + word.size();
        if (new_len <= max_line_length) {
            // still fits into line
            if (!line.empty())
                line.append(" ");
            line.append(word);
        } else {
            if (!line.empty())
                output_lines.push_back(line);

            line = word;
        }
    }

    if (!line.empty())
        output_lines.push_back(line);
}

static void join_strings(const vector<string> &lines, const string &joiner, string &output) {
    for (const string &a_line: lines) {
        if (!output.empty())
            output.append(joiner);

        output.append(a_line);
    }
}

QFileInfo find_unused_filename(const QFileInfo &output_directory,
                               const QString &basename,
                               const QString &extension,
                               const int tries) {
    // returns filepath like: "output_directory/[basename].[extension]" if it doesn't exist
    // otherwise tries "output_directory/[basename]_1.[extension]" up to `tries` attempts

    QString cnt_str("");
    for (int i = 0; i < tries; i++) {
        auto file_name = QString("%1%2.%3").arg(basename, cnt_str, extension);

        QFileInfo file_candidate(QDir(output_directory.absoluteFilePath()).absoluteFilePath(file_name));
        if (!file_candidate.exists())
            return file_candidate;

        cnt_str = QString("_%1").arg(i + 1);
    }

    throw string("nope");
}

QFileInfo find_transcript_filename_custom(const TranscriptOutputSettings &transcript_settings,
                                          const QFileInfo &output_directory,
                                          const bool is_stream,
                                          const std::chrono::system_clock::time_point &started_at,
                                          const int tries,
                                          bool &out_overwrite) {
    const string &name_custom = is_stream ? transcript_settings.streaming_filename_custom : transcript_settings.recording_filename_custom;
    const string &name_exists = is_stream ? transcript_settings.streaming_filename_exists
                                          : transcript_settings.recording_filename_exists;

    // fully user supplied name, don't add any extension
    if (name_custom.empty())
        throw string("custom filename chosen but no filename given");

    if (name_exists == "overwrite") {
        out_overwrite = true;
    }

    auto file = QFileInfo(QDir(output_directory.absoluteFilePath()).absoluteFilePath(QString::fromStdString(name_custom)));
    if (!file.exists())
        return file;

    if (name_exists == "overwrite") {
        out_overwrite = true;
        return file;
    }
    if (name_exists == "append") {
        out_overwrite = false;
        return file;
    }

    if (name_exists == "skip") {
        throw string("custom transcript file exists already, skipping: " + file.absoluteFilePath().toStdString());
    }

    throw string("custom transcript file exists already, invalid exists option: " + name_exists);
}

QFileInfo find_transcript_filename_datetime(const TranscriptOutputSettings &transcript_settings,
                                            const QFileInfo &output_directory,
                                            const bool is_stream,
                                            const std::chrono::system_clock::time_point &started_at,
                                            const int tries,
                                            const string &extension) {
    // "[streaming|recording]_transcript_2020-08-01_00-00-00[_cnt].[ext]"

    std::ostringstream oss;
    time_t started_at_t = std::chrono::system_clock::to_time_t(started_at);
    oss << std::put_time(std::localtime(&started_at_t), "%Y-%m-%d_%H-%M-%S");
    auto started_at_str = oss.str();

    string basename = is_stream ? "streaming_transcript_" : "recording_transcript_";
    basename = basename + started_at_str;
    return find_unused_filename(output_directory, QString::fromStdString(basename), QString::fromStdString(extension), tries);
}

QFileInfo find_transcript_filename_recording(const TranscriptOutputSettings &transcript_settings,
                                             const QFileInfo &output_directory,
                                             const std::chrono::system_clock::time_point &started_at,
                                             const int tries,
                                             const string &extension,
                                             const bool try_url) {
    // named like recording but with different extension (and optional cnt if it exists)

    OBSOutput output = obs_frontend_get_recording_output();
    obs_output_release(output);
    OBSData settings = obs_output_get_settings(output);
    obs_data_release(settings);

    string recording_path = obs_data_get_string(settings, "path");
    if (recording_path.empty()) {
        if (!try_url)
            throw string("couldn't get recording path");

        recording_path = obs_data_get_string(settings, "url");
        if (recording_path.empty())
            throw string("couldn't get recording url");

        info_log("find_transcript_filename_recording no recording path, using url");
    }

    const auto recording_file = QFileInfo(QString::fromStdString(recording_path));
    const QString basename = recording_file.completeBaseName();

    if (basename.isEmpty())
        throw string("couldn't get recording basename");

    return find_unused_filename(output_directory, basename, QString::fromStdString(extension), tries);
}

QFileInfo find_transcript_filename(const TranscriptOutputSettings &transcript_settings,
                                   const QFileInfo &output_directory,
                                   const bool is_stream,
                                   const std::chrono::system_clock::time_point &started_at,
                                   const int tries,
                                   bool &out_overwrite) {
    const string &name_type = is_stream ? transcript_settings.streaming_filename_type : transcript_settings.recording_filename_type;

    out_overwrite = false;
    if (name_type == "custom") {
        return find_transcript_filename_custom(transcript_settings, output_directory, is_stream, started_at, tries, out_overwrite);
    }

    const string extension = transcript_settings.format == "raw" ? "log" : transcript_settings.format;
    if (name_type == "datetime") {
        return find_transcript_filename_datetime(transcript_settings, output_directory, is_stream, started_at, tries, extension);
    }

    if (name_type == "recording" && !is_stream) {
        const int attempts = 1;
        const int sleep_ms = 1000;
        const bool fallback_datetime = true;

        for (int i = 0; i < attempts; i++) {
            if (i) {
                info_log("transcript_writer_loop find_transcript_filename recording retry, attempt %d, sleeping %dms", i, sleep_ms);
                std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
            }
            try {
                return find_transcript_filename_recording(transcript_settings, output_directory, started_at, tries, extension, true);
            }
            catch (string &err) {
                error_log("transcript_writer_loop find_transcript_filename recording error, try %d: %s", i, err.c_str());
            }
        }

        if (fallback_datetime) {
            info_log("transcript_writer_loop find_transcript_filename recording failed, falling back to datetime name");
            return find_transcript_filename_datetime(transcript_settings, output_directory, is_stream, started_at, tries, extension);
        }
        throw string("couldn't get recording basename after multiple tries");
    }

    throw string("unsupported name type: " + name_type);
}


string time_duration_stream(int milliseconds) {
//    printf("milliseconds %d\n", milliseconds);
    if (milliseconds < 0)
        return "";

    std::ostringstream out;
    out << std::setfill('0');

    uint hours = milliseconds / 3600000;
    milliseconds = milliseconds - (3600000 * hours);

    uint mins = milliseconds / 60000;
    milliseconds = milliseconds - (60000 * mins);

    uint secs = milliseconds / 1000;
    milliseconds = milliseconds - (1000 * secs);

    out << std::setw(2) << hours << ":" << std::setw(2) << mins << ":" << std::setw(2) << secs << "," << std::setw(3) << milliseconds;
    return out.str();
}

void fs_write_string(std::fstream &fs, const string &str) {
    fs << str;
    cerr << "writing:: " << str;
}

struct SrtState {
    std::chrono::steady_clock::time_point transcript_started_at;
    std::chrono::steady_clock::duration max_entry_duration;
    uint sequence_number = 0;
    uint line_length = 0;

    // only use caption results that were created up to this many milliseconds before the transcript was created
    // to prevent a sentence that had already started being spoken before the transcript started from going into the log
    // if it was started too long ago
    uint max_prestart_ms = 0;
};

int batch_up_to(const SrtState &settings, ResultQueue &results) {
    // returns index up to which the leading results can be combined into one
    if (results.empty())
        return -1;

    auto &first_received_at = results[0]->caption_result.first_received_at;

    uint take_until_ind = 0;
    for (int i = 1; i < results.size(); i++) {
        std::chrono::steady_clock::duration total = results[i]->caption_result.received_at - first_received_at;
        if (total <= settings.max_entry_duration)
            take_until_ind = i;
        else
            break;
    }
    return take_until_ind;
}

string srt_entry_caption_text(const SrtState &settings, const ResultQueue &results, const int up_to_index) {
    ostringstream text_os;
    for (int i = 0; i < up_to_index + 1; i++) {
        text_os << results[i]->clean_caption_text;
//        printf("using line %d, '%s'\n", i, results[i]->clean_caption_text.c_str());
        if (i)
            text_os << " ";
    }

    string text = text_os.str();
    text.erase(std::remove(text.begin(), text.end(), '\r'), text.end());
    std::replace(text.begin(), text.end(), '\n', ' ');
//    printf("replaced '%s'\n", text.c_str());

    if (settings.line_length) {
        vector<string> lines;
        split_into_non_wrapped_lines(lines, text, settings.line_length);
        text.clear();
        join_strings(lines, NEWLINE_STR, text);
    }

//    printf("lined %s\n", text.c_str());

    return text;
}

int write_results_batch_srt(const SrtState &settings, std::fstream &fs, const ResultQueue &results, const int up_to_index) {
    int start_offset_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            results[0]->caption_result.first_received_at - settings.transcript_started_at).count();

    if (start_offset_ms < 0)
        start_offset_ms = 0;

    string start_offset_str = time_duration_stream(start_offset_ms);
    if (start_offset_str.empty()) {
        return -1;
    }

    const int end_offset_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            results[up_to_index]->caption_result.received_at - settings.transcript_started_at).count();

    if (end_offset_ms < 0) {
        return 0;
    }

    const string end_offset_str = time_duration_stream(end_offset_ms);
    if (end_offset_str.empty()) {
        return -1;
    }

    const string entry_text = srt_entry_caption_text(settings, results, up_to_index);
    if (entry_text.empty()) {
        return 0;
    }

    std::ostringstream entry_os;
    entry_os << settings.sequence_number << NEWLINE_STR;
    entry_os << start_offset_str << " --> " << end_offset_str << NEWLINE_STR;
    entry_os << entry_text << NEWLINE_STR;
    entry_os << NEWLINE_STR;

    fs_write_string(fs, entry_os.str());
    return entry_text.size();
}

bool write_transcript_caption_results_srt(SrtState &settings, std::fstream &fs, ResultQueue &results, bool write_all) {
    // batch up multiple results into a single entry if the duration of all of them combined is less than settings.max_entry_duration

    const auto now = std::chrono::steady_clock::now();
    while (!results.empty() && !fs.fail()) {
        if (!write_all) {
            auto since_first = now - results[0]->caption_result.first_received_at;
            if (since_first <= settings.max_entry_duration) {
                debug_log(
                        "write_transcript_caption_results_srt: current results still too recent, batch could still change, "
                        "ignoring for now: %dms", (int) std::chrono::duration_cast<std::chrono::milliseconds>(since_first).count());
                break;
            }
        }

        const int up_to_index = batch_up_to(settings, results);
        if (up_to_index < 0)
            break;

        debug_log("write_transcript_caption_results_srt: using first %d items", up_to_index + 1);
        const int ret = write_results_batch_srt(settings, fs, results, up_to_index);
        if (ret > 0) {
            if (fs.fail()) {
                error_log("write_transcript_caption_results_srt: failed writing entry %d, %s", settings.sequence_number, strerror(errno));
                return false;
            }
            debug_log("write_transcript_caption_results_srt: wrote srt entry %d", settings.sequence_number);
            settings.sequence_number++;
            for (int i = 0; i < up_to_index + 1; i++) {
                results.pop_front();
            }
        } else if (ret == 0) {
            debug_log("write_transcript_caption_results_srt: skipped, still on entry %d", settings.sequence_number);
            for (int i = 0; i < up_to_index + 1; i++) {
                results.pop_front();
            }
        } else {
            error_log("write_transcript_caption_results_srt: failed writing entry %d", settings.sequence_number);
            return false;
        }
    }
    return true;
}

bool relevant_result(const SrtState &settings, const CaptionOutput &caption_output) {
    if (caption_output.output_result->clean_caption_text.empty())
        return false;

    int start_offset_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            caption_output.output_result->caption_result.first_received_at - settings.transcript_started_at).count();

    if (start_offset_ms < 0) {
        start_offset_ms = abs(start_offset_ms);
        if (!settings.max_prestart_ms || start_offset_ms > settings.max_prestart_ms) {
            debug_log("relevant_result: result from before transcript started, too much from before, %d", start_offset_ms);
            return false;
        }
    }

    int end_offset_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            caption_output.output_result->caption_result.received_at - settings.transcript_started_at).count();

    if (end_offset_ms < 0) {
        debug_log("relevant_result: result totally from before transcript started, ignore, %d", end_offset_ms);
        return false;
    }

    return true;
}

void write_loop_srt(SrtState &settings, std::fstream &fs,
                    shared_ptr<CaptionOutputControl<TranscriptOutputSettings>> control) {
    std::shared_ptr<OutputCaptionResult> held_nonfinal_result;
    CaptionOutput caption_output;
    ResultQueue results;

    while (!control->stop) {
        control->caption_queue.wait_dequeue(caption_output);

        if (control->stop)
            break;

        if (!caption_output.output_result || caption_output.is_clearance) {
            continue;
        }

        held_nonfinal_result = nullptr;
        if (!relevant_result(settings, caption_output))
            continue;;

        if (caption_output.output_result->caption_result.final) {
            results.push_back(caption_output.output_result);

            if (!write_transcript_caption_results_srt(settings, fs, results, false)) {
                return;
            }
        } else {
            held_nonfinal_result = caption_output.output_result;
        }
    }

    if (held_nonfinal_result) {
        results.push_back(held_nonfinal_result);
        held_nonfinal_result = nullptr;
    }

    if (!results.empty()) {
        if (!write_transcript_caption_results_srt(settings, fs, results, true)) {
            return;
        }
    }
}


void write_transcript_caption_simple(std::fstream &fs,
                                     const string &prefix,
                                     const std::chrono::steady_clock::time_point &started_at,
                                     const OutputCaptionResult &result) {
    int start_offset_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            result.caption_result.first_received_at - started_at).count();

    int end_offset_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            result.caption_result.received_at - started_at).count();

    if (start_offset_ms < 0)
        start_offset_ms = 0;

    if (end_offset_ms < 0)
        end_offset_ms = 0;

    const string start_offset_str = time_duration_stream(start_offset_ms);
    const string end_offset_str = time_duration_stream(end_offset_ms);

    std::ostringstream comb;
    comb << start_offset_str << "-" << end_offset_str << " " << prefix << "    " << result.clean_caption_text << std::endl;
    fs_write_string(fs, comb.str());
}

void write_loop_txt(std::fstream &fs, const std::chrono::steady_clock::time_point &started_at_steady,
                    shared_ptr<CaptionOutputControl<TranscriptOutputSettings>> control) {
    std::shared_ptr<OutputCaptionResult> held_nonfinal_caption_result;
    CaptionOutput caption_output;
    string prefix;
    while (!control->stop) {
        control->caption_queue.wait_dequeue(caption_output);

        if (control->stop)
            break;

        if (!caption_output.output_result || caption_output.is_clearance) {
//            debug_log("got empty CaptionOutput.output_result???");
            continue;
        }

        held_nonfinal_caption_result = nullptr;
        if (caption_output.output_result->caption_result.final) {
            write_transcript_caption_simple(fs, prefix, started_at_steady, *caption_output.output_result);
            if (fs.fail()) {
                error_log("transcript_writer_loop_txt error, write failed: '%s'", strerror(errno));
                return;
            }
        } else {
            held_nonfinal_caption_result = caption_output.output_result;
        }
    }

    if (held_nonfinal_caption_result) {
        write_transcript_caption_simple(fs, prefix, started_at_steady, *held_nonfinal_caption_result);
        if (fs.fail()) {
            error_log("transcript_writer_loop_txt error, write failed: '%s'", strerror(errno));
            return;
        }
    }
    held_nonfinal_caption_result = nullptr;
}

void write_loop_raw(std::fstream &fs, const std::chrono::steady_clock::time_point &started_at_steady,
                    shared_ptr<CaptionOutputControl<TranscriptOutputSettings>> control) {
    CaptionOutput caption_output;
    while (!control->stop) {
        control->caption_queue.wait_dequeue(caption_output);

        if (control->stop)
            break;

        if (!caption_output.output_result || caption_output.is_clearance) {
//            info_log("got empty CaptionOutput.output_result???");
            continue;
        }

        string prefix;
        if (caption_output.output_result->interrupted && caption_output.output_result->caption_result.final)
            prefix = " IF";
        else if (caption_output.output_result->interrupted)
            prefix = " I";
        else if (caption_output.output_result->caption_result.final)
            prefix = " F";

        write_transcript_caption_simple(fs, prefix, started_at_steady, *caption_output.output_result);
        if (fs.fail()) {
            error_log("transcript_writer_loop_raw error, write failed: '%s'", strerror(errno));
            return;
        }
    }
}

void transcript_writer_loop(shared_ptr<CaptionOutputControl<TranscriptOutputSettings>> control,
                            const bool to_stream,
                            const TranscriptOutputSettings transcript_settings) {
    const string format = transcript_settings.format;
    auto started_at_sys = std::chrono::system_clock::now();
    auto started_at_steady = std::chrono::steady_clock::now();

    string to_what(to_stream ? "streaming" : "recording");
    if (format != "txt" && format != "srt" && format != "raw") {
        error_log("transcript_writer_loop %s error, invalid format: %s", to_what.c_str(), format.c_str());
        return;
    }

    info_log("transcript_writer_loop %s starting, format: %s", to_what.c_str(), format.c_str());

    QFileInfo output_directory(QString::fromStdString(control->arg.output_path));
    if (!output_directory.exists()) {
        error_log("transcript_writer_loop %s error, output dir not found: %s", to_what.c_str(), control->arg.output_path.c_str());
        return;
    }
    if (!output_directory.isDir()) {
        error_log("transcript_writer_loop %s error, output dir not a directory: %s", to_what.c_str(), control->arg.output_path.c_str());
        return;
    }

    string transcript_file;
    bool overwrite_file = false;
    try {
        transcript_file = find_transcript_filename(transcript_settings, output_directory, to_stream, started_at_sys, 100, overwrite_file)
                .absoluteFilePath().toStdString();

        info_log("using transcript output file: '%s', overwrite existing: %d", transcript_file.c_str(), overwrite_file);
    }
    catch (string &err) {
        error_log("transcript_writer_loop find_transcript_filename error: %s", err.c_str());
        return;
    }
    catch (...) {
        error_log("transcript_writer_loop %s error, couldn't get an output filepath", to_what.c_str());
        return;
    }

    try {
        std::fstream fs;
        fs.open(transcript_file, std::fstream::out | std::ios::binary | (overwrite_file ? std::fstream::trunc : std::fstream::app));
        if (fs.fail()) {
            error_log("transcript_writer_loop %s error, couldn't open file", strerror(errno));
            return;
        }

        if (format == "raw")
            write_loop_raw(fs, started_at_steady, control);
        else if (format == "txt")
            write_loop_txt(fs, started_at_steady, control);
        else {
            const uint max_duration_secs = transcript_settings.srt_target_duration_secs ? transcript_settings.srt_target_duration_secs : 1;
            SrtState settings = SrtState{started_at_steady, std::chrono::seconds(max_duration_secs), 1,
                                         transcript_settings.srt_target_line_length, 1250};
            write_loop_srt(settings, fs, control);
        }

        fs.close();
    }
    catch (std::exception &ex) {
        error_log("transcript_writer_loop %s error %s", to_what.c_str(), ex.what());
        return;
    }
    catch (...) {
        error_log("transcript_writer_loop %s error", to_what.c_str());
        return;
    }

    info_log("transcript_writer_loop %s done", to_what.c_str());
}


#endif //OBS_GOOGLE_CAPTION_PLUGIN_CAPTION_TRANSCRIPT_WRITER_H
