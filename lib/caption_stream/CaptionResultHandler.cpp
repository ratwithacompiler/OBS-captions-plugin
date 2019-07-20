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


#include "CaptionResultHandler.h"
#include "log.h"
#include <sstream>
#include <iostream>
#include <vector>
#include <json11.hpp>
#include <utils.h>

using namespace json11;


shared_ptr<CaptionResult> parse_caption_obj(const string &msg_obj) {
    bool is_final = false;
    double highest_stability = 0.0;
    string caption_text;
    int result_index;

    string err;
    Json json = Json::parse(msg_obj, err);
    if (!err.empty()) {
        throw err;
    }
    Json result_val = json["result"];
    if (!result_val.is_array())
        throw string("no result");

    if (!result_val.array_items().size())
        throw string("empty");

    Json result_index_val = json["result_index"];
    if (!result_index_val.is_number())
        throw string("no result index");

    result_index = result_index_val.int_value();

    for (auto &result_obj : result_val.array_items()) {
        string text = result_obj["alternative"][0]["transcript"].string_value();
        if (text.empty())
            continue;

        if (result_obj["final"].bool_value()) {
            is_final = true;
            caption_text = text;
            break;
        }

        Json stability_val = result_obj["stability"];
        if (!stability_val.is_number())
            continue;

        if (stability_val.number_value() >= highest_stability) {
            caption_text = text;
            highest_stability = stability_val.number_value();
        }
//        info_log("val %s", text_val.dump().c_str());

//        printf("Result: %s\n", json.dump().c_str());
//        cout<< a.dump() << endl;
//        printf("Result: %s\n", json.dump().c_str());
    }

    if (caption_text.empty())
        throw string("no caption text");

//    debug_log("stab: %f, final: %d, index: %d, text: '%s'", highest_stability, is_final, result_index, caption_text.c_str());
    return std::make_shared<CaptionResult>(result_index, is_final, highest_stability, caption_text);
}

vector<string> split_into_non_wrapped_lines(const string &text, const uint max_line_length) {
    vector<string> lines;

    istringstream stream(text);
    string word;
    string line;
    while (getline(stream, word, ' ')) {
        if (word.empty())
            continue;

        int new_len = line.size() + (line.empty() ? 0 : 1) + word.size();
        if (new_len <= max_line_length) {
            // still finds into line
            if (!line.empty())
                line.append(" ");
            line.append(word);
        } else {
            if (!line.empty())
                lines.push_back(line);

            line = word;
        }
    }

    if (!line.empty())
        lines.push_back(line);

//    for (auto line: lines)
//        debug_log("%s", line.c_str());

    return lines;
}


shared_ptr<CaptionResult> CaptionResultHandler::parse_caption_object(
        const string &caption_obj,
        bool fillup_with_previous
) {
    try {
        auto result = parse_caption_obj(caption_obj);
//        debug_log("caption: %s", res.caption_text.c_str());

        const uint targeted_line_count = settings.caption_line_count;
        const uint line_length = settings.caption_line_length;
        const uint max_length = targeted_line_count * line_length;

        if (!settings.manual_banned_words.empty()) {
            string cleaned;
            int removed = word_filter(result->caption_text, cleaned, settings.manual_banned_words);
            if (removed) {
                info_log("removed %d manual banned words, %s", removed, result->caption_text.c_str());
                result->caption_text = cleaned;
            }
        }

        if (!settings.default_banned_words.empty()) {
            string cleaned;
            int removed = word_filter(result->caption_text, cleaned, settings.default_banned_words);
            if (removed) {
                info_log("removed %d default banned words, %s", removed, result->caption_text.c_str());
                result->caption_text = cleaned;
            }
        }

        vector<string> all_lines;
        if (
                fillup_with_previous
                && result->caption_text.size() <= max_length
                && last_final_result
                && !last_final_result->caption_text.empty()
                ) {

            bool fill = true;
            if (settings.caption_timeout_enabled) {
                double secs_since_last = std::chrono::duration_cast<std::chrono::duration<double >>
                        (std::chrono::steady_clock::now() - last_final_result->created_at).count();

                fill = bool(secs_since_last <= settings.caption_timeout_seconds);
//                if (!fill) {
//                    debug_log("not filling, too old %f >= %f", secs_since_last, settings.caption_timeout_seconds);
//                }
            }

            if (fill) {
//                debug_log("filled up with previous text %lu, %lu", result->caption_text.size(), last_final_result->caption_text.size());
                string prefixed_text(last_final_result->caption_text);
                prefixed_text.append(result->caption_text);
                all_lines = split_into_non_wrapped_lines(prefixed_text, line_length);
            } else {
                all_lines = split_into_non_wrapped_lines(result->caption_text, line_length);
            }
        } else {
            all_lines = split_into_non_wrapped_lines(result->caption_text, line_length);
        }

        const uint use_lines_cnt = all_lines.size() > targeted_line_count ? targeted_line_count : all_lines.size();
        result->output_lines.insert(result->output_lines.end(), all_lines.end() - use_lines_cnt, all_lines.end());
        result->last_final_result = last_final_result;

        if (result->final) {
            last_final_result = result;
        }

        if (last_line == result->caption_text) {
//            debug_log("duplicate");
            return nullptr;
        }
        last_line = result->caption_text;

//        debug_log("lines: %d", use_lines->size());
//        for (const auto &line: use_lines)
//            debug_log("line: %s", line.c_str());
//        debug_log("");
        return result;

    } catch (string &ex) {
        info_log("couldn't parse caption message. Error: '%s'. Messsage: '%s'", ex.c_str(), caption_obj.c_str());
        return nullptr;
    }
    catch (...) {
        info_log("couldn't parse caption message. Messsage: '%s'", caption_obj.c_str());
        return nullptr;
    }
}

void CaptionResultHandler::clear_history() {
    if (last_final_result) {
        last_final_result = nullptr;
        last_line.clear();
    }

}

CaptionResultHandler::CaptionResultHandler(CaptionFormatSettings settings) :
        settings(settings) {

//    for (int i = 0; i < banned_words.size(); i++) {
//        info_log("banned word %d, %s", i, banned_words[i].c_str());
//    }
}
