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
#include "SourceCaptioner.h"

QFileInfo find_transcript_filename(const QFileInfo &output_directory,
                                   const QString &prefix,
                                   const QString &extension,
                                   const std::chrono::system_clock::time_point &started_at,
                                   int tries) {
    // returns filepath like: "output_directory/prefix_date.txt" if it doesn't exist
    // otherwise tries "output_directory/prefix_date_0.txt", "output_directory/prefix_date_1.txt"up to `tries` attempts

    std::ostringstream oss;
    time_t started_at_t = std::chrono::system_clock::to_time_t(started_at);
    oss << std::put_time(std::localtime(&started_at_t), "%Y-%m-%d_%H-%M-%S");
    auto started_at_str = oss.str();

    QString cnt_str("");
    for (int i = 0; i < tries; i++) {
        auto file_name = QString("%1_transcript_%2%3%4").arg(prefix, QString::fromStdString(started_at_str), cnt_str, extension);

        QFileInfo file_candidate(QDir(output_directory.absoluteFilePath()).absoluteFilePath(file_name));
        if (!file_candidate.exists())
            return file_candidate;

        cnt_str = QString("_%1").arg(i + 1);
    }

    throw string("nope");
}


string time_duration_stream(bool srt, std::chrono::steady_clock::duration diff) {
    std::ostringstream out;
    int millis_since = std::chrono::duration_cast<std::chrono::milliseconds>(diff).count();
    if (millis_since < 0)
        out << "-";

    out << std::setfill('0');

    uint hours = millis_since / 3600000;
    millis_since = millis_since - (3600000 * hours);

    uint mins = millis_since / 60000;
    millis_since = millis_since - (60000 * mins);

    uint secs = millis_since / 1000;
    millis_since = millis_since - (1000 * secs);

    out << std::setw(2) << hours << ":" << std::setw(2) << mins << ":" << std::setw(2) << secs << (srt ? "," : ".") << millis_since;
    return out.str();
}

void write_transcript_caption(std::fstream &fs,
                              const string &prefix,
                              const std::chrono::steady_clock::time_point &started_at,
                              const OutputCaptionResult &result, bool srt) {
    if (result.clean_caption_text.empty()) {
        debug_log("write_transcript_caption ignoring empty");
        return;
    }
    string end_since_start_str = time_duration_stream(srt, result.caption_result.created_at - started_at);
    string start_since_start_str   = time_duration_stream(srt, result.caption_result.start_at - started_at);
    if (srt) 
        fs << result.caption_result.index << "\n"
           << start_since_start_str << " --> " << end_since_start_str << "\n"
           << (prefix.empty() ? "" : prefix +"\n")
           << result.clean_caption_text << "\n" << std::endl;
    else
        fs << end_since_start_str << prefix << "    " << result.clean_caption_text << std::endl;
    
    cerr << "writing:: " << end_since_start_str << "    " << result.clean_caption_text << std::endl;

}

void write_loop_txt(std::fstream &fs, const std::chrono::steady_clock::time_point &started_at_steady,
                    shared_ptr<CaptionOutputControl<TranscriptOutputSettings>> control, bool srt) {
    std::shared_ptr<OutputCaptionResult> held_nonfinal_caption_result;
    CaptionOutput caption_output;
    string prefix;
    while (!control->stop) {
        control->caption_queue.wait_dequeue(caption_output);

        if (control->stop)
            break;

        if (!caption_output.output_result) {
            info_log("got empty CaptionOutput.output_result???");
            continue;
        }

        if (caption_output.interrupted) {
            if (held_nonfinal_caption_result) {
                write_transcript_caption(fs, prefix, started_at_steady, *held_nonfinal_caption_result, srt);
                if (fs.fail()) {
                    error_log("transcript_writer_loop_txt error, write failed: '%s'", strerror(errno));
                    return;
                }
            }
        }
        held_nonfinal_caption_result = nullptr;

        if (caption_output.output_result->caption_result.final) {
            write_transcript_caption(fs, prefix, started_at_steady, *caption_output.output_result, srt);
            if (fs.fail()) {
                error_log("transcript_writer_loop_txt error, write failed: '%s'", strerror(errno));
                return;
            }
        } else {
            held_nonfinal_caption_result = caption_output.output_result;
        }
    }

    if (held_nonfinal_caption_result) {
        write_transcript_caption(fs, prefix, started_at_steady, *held_nonfinal_caption_result, srt);
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

        if (!caption_output.output_result) {
            info_log("got empty CaptionOutput.output_result???");
            continue;
        }

        string prefix;
        if (caption_output.interrupted && caption_output.output_result->caption_result.final)
            prefix = " IF";
        else if (caption_output.interrupted)
            prefix = " I";
        else if (caption_output.output_result->caption_result.final)
            prefix = " F";

        write_transcript_caption(fs, prefix, started_at_steady, *caption_output.output_result, false);
        if (fs.fail()) {
            error_log("transcript_writer_loop_raw error, write failed: '%s'", strerror(errno));
            return;
        }
    }
}

void transcript_writer_loop(shared_ptr<CaptionOutputControl<TranscriptOutputSettings>> control, bool to_stream, string output_format) {
    auto started_at_sys = std::chrono::system_clock::now();
    auto started_at_steady = std::chrono::steady_clock::now();

    string to_what(to_stream ? "streaming" : "recording");
    info_log("transcript_writer_loop %s starting, output format: %s", to_what.c_str(), output_format);

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
    try {
        auto extension = QString(output_format == "raw" ? ".raw.log" : (output_format == "srt" ? ".srt" : ".txt"));
        transcript_file = find_transcript_filename(output_directory, QString::fromStdString(to_what), extension, started_at_sys,
                                                   100).absoluteFilePath().toStdString();

        info_log("using transcript output file: %s", transcript_file.c_str());
    }
    catch (...) {
        error_log("transcript_writer_loop %s error, couldn't get an output filepath", to_what.c_str());
        return;
    }

    try {
        std::fstream fs;
        fs.open(transcript_file, std::fstream::out | std::fstream::app);
        if (fs.fail()) {
            error_log("transcript_writer_loop %s error, couldn't open file", strerror(errno));
            return;
        }

        if (output_format == "raw")
            write_loop_raw(fs, started_at_steady, control);
        else if (output_format == "srt")
                write_loop_txt(fs, started_at_steady, control,true);
            else 
                write_loop_txt(fs, started_at_steady, control,false);
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
