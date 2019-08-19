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
#include <utils.h>

void split_into_non_wrapped_lines(vector<string> &output_lines, const string &text, const uint max_line_length) {
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

//    for (auto line: output_lines)
//        debug_log("%s", line.c_str());
}


shared_ptr<OutputCaptionResult> CaptionResultHandler::prepare_caption_output(
        const CaptionResult &caption_result,
        bool fillup_with_previous,
        const std::vector<std::shared_ptr<OutputCaptionResult>> &result_history
) {

    shared_ptr<OutputCaptionResult> output_result = make_shared<OutputCaptionResult>(caption_result);

    try {
//        debug_log("caption: %s", res.caption_text.c_str());

        const uint targeted_line_count = settings.caption_line_count;
        const uint line_length = settings.caption_line_length;
        const uint max_length = targeted_line_count * line_length;

        string cleaned_line = caption_result.caption_text;

        if (!settings.manual_banned_words.empty()) {
            string tmp_cleaned;
            int removed = word_filter(cleaned_line, tmp_cleaned, settings.manual_banned_words);
            if (removed) {
                info_log("removed %d manual banned words, %s", removed, cleaned_line.c_str());
                cleaned_line = tmp_cleaned;
            }
        }

        if (!settings.default_banned_words.empty()) {
            string tmp_cleaned;
            int removed = word_filter(cleaned_line, tmp_cleaned, settings.default_banned_words);
            if (removed) {
                info_log("removed %d default banned words, %s", removed, cleaned_line.c_str());
                cleaned_line = tmp_cleaned;
            }
        }

        output_result->clean_caption_text = cleaned_line;

        vector<string> all_lines;
        if (fillup_with_previous) {
            string filled_line = cleaned_line;

            if (filled_line.size() < max_length && !result_history.empty()) {
                for (auto i = result_history.rbegin(); i != result_history.rend(); ++i) {
                    if (!*i)
                        break;

                    if (!(*i)->caption_result.final)
                        // had interruption here, ignore
                        break;

                    if (settings.caption_timeout_enabled) {
                        double secs_since_last = std::chrono::duration_cast<std::chrono::duration<double >>
                                (std::chrono::steady_clock::now() - (*i)->caption_result.created_at).count();

                        if (secs_since_last > settings.caption_timeout_seconds) {
//                            debug_log("not filling, too old %f >= %f", secs_since_last, settings.caption_timeout_seconds);
                            break;
                        }
                    }

                    filled_line.insert(0, 1, ' ');
                    filled_line.insert(0, (*i)->clean_caption_text);

//                    debug_log("filled up with previous text %lu, %lu", filled_line.size(), (*i)->clean_caption_text.size());
//                    debug_log("filled up with previous text added '%s', filled: '%s'",
//                              (*i)->clean_caption_text.c_str(), filled_line.c_str());

                    if (filled_line.size() >= max_length)
                        break;
                }
            }
            split_into_non_wrapped_lines(all_lines, filled_line, line_length);
        } else {
            split_into_non_wrapped_lines(all_lines, cleaned_line, line_length);
        }

        const uint use_lines_cnt = all_lines.size() > targeted_line_count ? targeted_line_count : all_lines.size();
        output_result->output_lines.insert(output_result->output_lines.end(), all_lines.end() - use_lines_cnt, all_lines.end());

//        debug_log("lines: %lu", output_result->output_lines.size());
//        for (const auto &line: output_result->output_lines)
//            debug_log("line: %s", line.c_str());
//        debug_log("");

        return output_result;

    } catch (string &ex) {
        info_log("couldn't parse caption message. Error: '%s'. Messsage: '%s'", ex.c_str(), caption_result.caption_text.c_str());
        return nullptr;
    }
    catch (...) {
        info_log("couldn't parse caption message. Messsage: '%s'", caption_result.caption_text.c_str());
        return nullptr;
    }
}


CaptionResultHandler::CaptionResultHandler(CaptionFormatSettings settings) :
        settings(settings) {

//    for (int i = 0; i < banned_words.size(); i++) {
//        info_log("banned word %d, %s", i, banned_words[i].c_str());
//    }
}
