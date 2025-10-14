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
#include "ui/uiutils.h"
#include "stringutils.h"

using ResultTup = std::tuple<
        std::chrono::steady_clock::time_point,
        std::chrono::steady_clock::time_point,
        std::string,
        bool   //is_sentence_start: false for parts[1:] of a sentence that was split into multiple parts
        // so those can be handled differently from normal single full sentences.
>;
using ResultQueue = std::deque<ResultTup>;

const string NEWLINE_STR = "\r\n";

using MonoTP = std::chrono::steady_clock::time_point;
using MonoDur = std::chrono::steady_clock::duration;


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

struct UseTranscriptSettings {
    string name_type;
    string filename_custom;
    string filename_custom_exists;
    string file_basename;
};

UseTranscriptSettings build_use_settings(const TranscriptOutputSettings &transcript_settings, const string &target_name) {
    string name_type;
    string name_custom;
    string name_exists;
    string basename;

    if (target_name == "stream") {
        name_type = transcript_settings.streaming_filename_type;
        name_custom = transcript_settings.streaming_filename_custom;
        name_exists = transcript_settings.streaming_filename_exists;
        basename = "streaming_transcript_";
    } else if (target_name == "recording") {
        name_type = transcript_settings.recording_filename_type;
        name_custom = transcript_settings.recording_filename_custom;
        name_exists = transcript_settings.recording_filename_exists;
        basename = "recording_transcript_";
    } else if (target_name == "virtualcam") {
        name_type = transcript_settings.virtualcam_filename_type;
        name_custom = transcript_settings.virtualcam_filename_custom;
        name_exists = transcript_settings.virtualcam_filename_exists;
        basename = "virtualcam_transcript_";
    } else
        throw string("unsupported target name: " + target_name);

    return UseTranscriptSettings{
            name_type,
            name_custom,
            name_exists,
            basename,
    };
}

QFileInfo find_transcript_filename_custom(const TranscriptOutputSettings &transcript_settings,
                                          const UseTranscriptSettings &rel,
                                          const QFileInfo &output_directory,
                                          const std::chrono::system_clock::time_point &started_at,
                                          const int tries,
                                          bool &out_overwrite) {

    // fully user supplied name, don't add any extension
    if (rel.filename_custom.empty())
        throw string("custom filename chosen but no filename given");

    if (rel.filename_custom_exists == "overwrite") {
        out_overwrite = true;
    }

    auto file = QFileInfo(QDir(output_directory.absoluteFilePath()).absoluteFilePath(QString::fromStdString(rel.filename_custom)));
    if (!file.exists())
        return file;

    if (rel.filename_custom_exists == "overwrite") {
        out_overwrite = true;
        return file;
    }
    if (rel.filename_custom_exists == "append") {
        out_overwrite = false;
        return file;
    }

    if (rel.filename_custom_exists == "skip") {
        throw string("custom transcript file exists already, skipping: " + file.absoluteFilePath().toStdString());
    }

    throw string("custom transcript file exists already, invalid exists option: " + rel.filename_custom_exists);
}

QFileInfo find_transcript_filename_datetime(const TranscriptOutputSettings &transcript_settings,
                                            const UseTranscriptSettings &rel,
                                            const QFileInfo &output_directory,
                                            const std::chrono::system_clock::time_point &started_at,
                                            const int tries,
                                            const string &extension) {
    // "[streaming|recording]_transcript_2020-08-01_00-00-00[_cnt].[ext]"

    std::ostringstream oss;
    time_t started_at_t = std::chrono::system_clock::to_time_t(started_at);
    oss << std::put_time(std::localtime(&started_at_t), "%Y-%m-%d_%H-%M-%S");
    auto started_at_str = oss.str();


    string basename = rel.file_basename + started_at_str;
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
                                   const UseTranscriptSettings &rel,
                                   const QFileInfo &output_directory,
                                   const string &target_name,
                                   const std::chrono::system_clock::time_point &started_at,
                                   const int tries,
                                   bool &out_overwrite) {
    out_overwrite = false;
    if (rel.name_type == "custom") {
        return find_transcript_filename_custom(transcript_settings, rel, output_directory, started_at, tries, out_overwrite);
    }

    const string extension = transcript_format_extension(transcript_settings.format, "");
    if (rel.name_type == "datetime") {
        return find_transcript_filename_datetime(transcript_settings, rel, output_directory, started_at, tries, extension);
    }

    if (rel.name_type == "recording" && target_name == "recording") {
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
            catch (...) {
                error_log("transcript_writer_loop find_transcript_filename recording error, try %d", i);
            }
        }

        if (fallback_datetime) {
            info_log("transcript_writer_loop find_transcript_filename recording failed, falling back to datetime name");
            return find_transcript_filename_datetime(transcript_settings, rel, output_directory, started_at, tries, extension);
        }
        throw string("couldn't get recording basename after multiple tries");
    }

    throw string("unsupported name type: " + rel.name_type);
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
    bool add_punctuation = false;
    bool split_sentence = false;
    bool write_realtime = false;
    CapitalizationType capitalization = CAPITALIZATION_NORMAL;

    // only use caption results that were created up to this many milliseconds before the transcript was created
    // to prevent a sentence that had already started being spoken before the transcript started from going into the log
    // if it was started too long ago
    uint max_prestart_ms = 0;
};

int batch_up_to(const SrtState &settings, ResultQueue &results) {
    // returns index up to which the leading results can be combined into one
    if (results.empty())
        return -1;

    auto &first_received_at = std::get<0>(results[0]);

    uint take_until_ind = 0;
    for (int i = 1; i < results.size(); i++) {
        std::chrono::steady_clock::duration total = std::get<1>(results[i]) - first_received_at;
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
        string line = std::get<2>(results[i]);
        if (settings.add_punctuation
            && settings.capitalization == CAPITALIZATION_NORMAL
            && std::get<3>(results[i]) //only capitalize the first part
            && !line.empty()
            && isascii(line[0])) {
            line[0] = toupper(line[0]);
        }

        //printf("SRT: using line %d, '%s'\n", i, line.c_str());
        if (i) {
            if (settings.add_punctuation && std::get<3>(results[i])) {
                text_os << ".";
            }
            text_os << " ";
        }
        text_os << line;
    }

    string text = text_os.str();
    text.erase(std::remove(text.begin(), text.end(), '\r'), text.end());
    std::replace(text.begin(), text.end(), '\n', ' ');
//    printf("replaced '%s'\n", text.c_str());

    if (settings.line_length) {
        vector<string> lines;
        split_into_lines(lines, text, settings.line_length);
        text.clear();
        join_strings(lines, NEWLINE_STR, text);
    }

//    printf("lined %s\n", text.c_str());

    return text;
}

int write_results_batch_srt(const SrtState &settings, std::fstream &fs, const ResultQueue &results, const int up_to_index) {
    int start_offset_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::get<0>(results[0]) - settings.transcript_started_at).count();

    if (start_offset_ms < 0)
        start_offset_ms = 0;

    string start_offset_str = time_duration_stream(start_offset_ms);
    if (start_offset_str.empty()) {
        return -1;
    }

    const int end_offset_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::get<1>(results[up_to_index]) - settings.transcript_started_at).count();

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
    int wrote_cnt = 0;
    while (!results.empty() && !fs.fail()) {
        if (!write_all) {
            auto since_first = now - std::get<0>(results[0]);
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
        wrote_cnt++;
    }
    if (settings.write_realtime && wrote_cnt) {
        fs.flush();
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


void split_vec(std::vector<std::vector<string>> &out, const std::vector<string> &words, size_t chunks) {
    size_t length = words.size() / chunks;
    size_t extra = words.size() % chunks;

    size_t begin = 0;
    size_t end = 0;
    for (size_t i = 0; i < (std::min)(chunks, words.size()); ++i) {
        end += length;
        if (extra > 0) {
            end += 1;
            extra--;
        }
        out.push_back(std::vector<string>(words.begin() + begin, words.begin() + end));
        begin = end;
    }
}

void split_res(const MonoTP &start, const MonoTP &end, const string &full_text,
               const MonoDur &max_duration, ResultQueue &results) {

    MonoDur length = end - start;

    double parts = (double) (std::chrono::duration_cast<std::chrono::milliseconds>(length).count()) /
                   (double) (std::chrono::duration_cast<std::chrono::milliseconds>(max_duration).count());
    int parts_cnt = ceil(parts);
    debug_log("sentence longer than max duration, %lld >= %lld, splitting into %d parts (src %f)",
              std::chrono::duration_cast<std::chrono::milliseconds>(length).count(),
              std::chrono::duration_cast<std::chrono::milliseconds>(max_duration).count(),
                      parts_cnt, parts);

    if (parts_cnt <= 1) {
//        printf("split_res parts_cnt <= 1, no point: %d\n", parts_cnt);
        results.emplace_back(start, end, full_text, true);
        return;
    }

    QString qtext = QString::fromStdString(full_text).simplified();
    QStringList qwords = qtext.split(QRegularExpression("\\s+"));
    std::vector<string> words;
    for (int i = 0; i < qwords.size(); i++)
        words.push_back(qwords.at(i).toStdString());

    std::vector<std::vector<string>> word_vecs;
    split_vec(word_vecs, words, parts_cnt);

    MonoDur part_duration = length / parts_cnt;
    int cnt = 0;
    for (const auto &word_vec: word_vecs) {
        string line;
        join_strings(word_vec, " ", line);
        if (!line.empty()) {
            MonoDur part_start = part_duration * cnt;
            MonoDur part_end = part_duration * (cnt + 1);
            results.emplace_back(start + part_start, start + part_end, line, cnt == 0);
        }
        cnt++;
    }
}

void add_result(SrtState &settings, ResultQueue &results, shared_ptr<OutputCaptionResult> output_res) {
    string text = output_res->clean_caption_text;
    string_capitalization(text, settings.capitalization);

    bool split = false;
    if (settings.split_sentence) {
        std::chrono::steady_clock::duration length = output_res->caption_result.received_at
                                                     - output_res->caption_result.first_received_at;
        split = length > settings.max_entry_duration;
    }

    if (!split) {
        results.emplace_back(output_res->caption_result.first_received_at, output_res->caption_result.received_at,
                             text, true);
        return;
    }

//    auto before = results.size();
    split_res(output_res->caption_result.first_received_at, output_res->caption_result.received_at,
              text, settings.max_entry_duration, results);

//    int cnt = 0;
//    printf("added lines: %lu\n", results.size() - before);
//    for (int i = before; i < results.size(); i++) {
//        const auto &res = results[i];
//        printf("line %d:  %lld -> %lld: '%s' \n", cnt++,
//               std::chrono::duration_cast<std::chrono::milliseconds>(std::get<0>(res) - settings.transcript_started_at).count(),
//               std::chrono::duration_cast<std::chrono::milliseconds>(std::get<1>(res) - settings.transcript_started_at).count(),
//               std::get<2>(res).c_str()
//        );
//    }
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
            continue;

        if (caption_output.output_result->caption_result.final) {
            add_result(settings, results, caption_output.output_result);

            if (!write_transcript_caption_results_srt(settings, fs, results, false)) {
                return;
            }
        } else {
            held_nonfinal_result = caption_output.output_result;
        }
    }

    if (held_nonfinal_result) {
        add_result(settings, results, held_nonfinal_result);
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
                                     const OutputCaptionResult &result,
                                     const bool add_timestamps) {
    std::ostringstream comb;
    if (add_timestamps) {
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
        comb << start_offset_str << "-" << end_offset_str << " ";
    }

    if (!prefix.empty())
        comb << prefix;

    comb << result.clean_caption_text << std::endl;
    fs_write_string(fs, comb.str());
}

void write_loop_txt(SrtState &settings, std::fstream &fs, const std::chrono::steady_clock::time_point &started_at_steady,
                    shared_ptr<CaptionOutputControl<TranscriptOutputSettings>> control,
                    const bool add_timestamps, const bool add_spacer) {
    std::shared_ptr<OutputCaptionResult> held_nonfinal_caption_result;
    CaptionOutput caption_output;
    const string prefix(add_spacer ? "    " : "");

    while (!control->stop) {
        control->caption_queue.wait_dequeue(caption_output);

        if (control->stop)
            break;

        if (!caption_output.output_result || caption_output.is_clearance) {
//            debug_log("got empty CaptionOutput.output_result???");
            continue;
        }

        held_nonfinal_caption_result = nullptr;
        if (!relevant_result(settings, caption_output))
            continue;

        if (caption_output.output_result->caption_result.final) {
            write_transcript_caption_simple(fs, prefix, started_at_steady, *caption_output.output_result, add_timestamps);
            if (settings.write_realtime) {
                fs.flush();
            }
            if (fs.fail()) {
                error_log("transcript_writer_loop_txt error, write failed: '%s'", strerror(errno));
                return;
            }
        } else {
            held_nonfinal_caption_result = caption_output.output_result;
        }
    }

    if (held_nonfinal_caption_result) {
        write_transcript_caption_simple(fs, prefix, started_at_steady, *held_nonfinal_caption_result, add_timestamps);
        if (settings.write_realtime) {
            fs.flush();
        }
        if (fs.fail()) {
            error_log("transcript_writer_loop_txt error, write failed: '%s'", strerror(errno));
            return;
        }
    }
    held_nonfinal_caption_result = nullptr;
}

void write_loop_raw(std::fstream &fs, const std::chrono::steady_clock::time_point &started_at_steady, bool write_realtime,
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
            prefix = "IF    ";
        else if (caption_output.output_result->interrupted)
            prefix = "I    ";
        else if (caption_output.output_result->caption_result.final)
            prefix = "F    ";
        else
            prefix = "    ";

        write_transcript_caption_simple(fs, prefix, started_at_steady, *caption_output.output_result, true);
        if (write_realtime) {
            fs.flush();
        }
        if (fs.fail()) {
            error_log("transcript_writer_loop_raw error, write failed: '%s'", strerror(errno));
            return;
        }
    }
}

void transcript_writer_loop(shared_ptr<CaptionOutputControl<TranscriptOutputSettings>> control,
                            const string target_name, const TranscriptOutputSettings transcript_settings) {
    const string format = transcript_settings.format;
    auto started_at_sys = std::chrono::system_clock::now();
    auto started_at_steady = std::chrono::steady_clock::now();
    const string to_what = target_name;

    UseTranscriptSettings use_settings;
    try {
        use_settings = build_use_settings(transcript_settings, target_name);
    } catch (string ex) {
        error_log("transcript_writer_loop startup failed: %s %s", to_what.c_str(), ex.c_str());
        return;
    } catch (...) {
        error_log("transcript_writer_loop startup failed: %s", to_what.c_str());
        return;
    }

    if (format != "txt" && format != "txt_plain" && format != "srt" && format != "raw") {
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

    QString transcript_file;
    bool overwrite_file = false;
    try {
        transcript_file = find_transcript_filename(transcript_settings, use_settings, output_directory, target_name, started_at_sys, 100,
                                                   overwrite_file)
                .absoluteFilePath();
        info_log("using transcript output file: '%s', overwrite existing: %d", transcript_file.toStdString().c_str(), overwrite_file);
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

#if _WIN32
        fs.open(transcript_file.toStdWString(), std::fstream::out | std::ios::binary | (overwrite_file ? std::fstream::trunc : std::fstream::app));
#else
        fs.open(transcript_file.toStdString(),
                std::fstream::out | std::ios::binary | (overwrite_file ? std::fstream::trunc : std::fstream::app));
#endif

        if (fs.fail()) {
            error_log("transcript_writer_loop %s error, couldn't open file", strerror(errno));
            return;
        }

        const uint max_duration_secs = transcript_settings.srt_target_duration_secs ? transcript_settings.srt_target_duration_secs : 1;
        SrtState settings = SrtState{started_at_steady, std::chrono::seconds(max_duration_secs), 1,
                                     transcript_settings.srt_target_line_length,
                                     transcript_settings.srt_add_punctuation, transcript_settings.srt_split_single_sentences,
                                     transcript_settings.write_realtime,
                                     transcript_settings.srt_capitalization, 1250};

        info_log("transcript_writer_loop starting write_loop: %s", format.c_str());
        if (format == "raw")
            write_loop_raw(fs, started_at_steady, transcript_settings.write_realtime, control);
        else if (format == "txt")
            write_loop_txt(settings, fs, started_at_steady, control, true, true);
        else if (format == "txt_plain")
            write_loop_txt(settings, fs, started_at_steady, control, false, false);
        else {
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
