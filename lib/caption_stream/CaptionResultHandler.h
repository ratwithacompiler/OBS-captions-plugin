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


#include <utility>

#ifndef CPPTESTING_CAPTIONRESULTHANDLER_H
#define CPPTESTING_CAPTIONRESULTHANDLER_H

#include <string>
#include "log.h"
#include "CaptionStream.h"
#include <json11.hpp>


struct CaptionFormatSettings {
    uint caption_line_length;
    uint caption_line_count;
    bool caption_insert_newlines;
    std::vector<string> default_banned_words;
    std::vector<string> manual_banned_words;

    bool caption_timeout_enabled;
    double caption_timeout_seconds;

    CaptionFormatSettings(
            uint caption_line_length,
            uint caption_line_count,
            bool caption_insert_newlines,
            std::vector<string> default_banned_words,
            bool caption_timeout_enabled,
            double caption_timeout_seconds
    ) :
            caption_line_length(caption_line_length),
            caption_line_count(caption_line_count),
            caption_insert_newlines(caption_insert_newlines),
            default_banned_words(default_banned_words),
            caption_timeout_enabled(caption_timeout_enabled),
            caption_timeout_seconds(caption_timeout_seconds) {
        info_log("CaptionFormatSettings banned words: %lu %lu, ", default_banned_words.size(), manual_banned_words.size());
    }

    void print() {
        printf("CaptionFormatSettings\n");
        printf("    caption_line_length: %d\n", caption_line_length);
        printf("    caption_line_count: %d\n", caption_line_count);
        printf("    caption_insert_newlines: %d\n", caption_insert_newlines);
        printf("    manual_banned_words: %lu\n", manual_banned_words.size());
        for (auto &word : manual_banned_words)
            printf("        '%s'\n", word.c_str());

        printf("-----------");
    }

    bool operator==(const CaptionFormatSettings &rhs) const {
        return caption_line_length == rhs.caption_line_length &&
               caption_line_count == rhs.caption_line_count &&
               caption_insert_newlines == rhs.caption_insert_newlines &&
               default_banned_words == rhs.default_banned_words &&
               manual_banned_words == rhs.manual_banned_words &&
               caption_timeout_enabled == rhs.caption_timeout_enabled &&
               caption_timeout_seconds == rhs.caption_timeout_seconds;
    }

    bool operator!=(const CaptionFormatSettings &rhs) const {
        return !(rhs == *this);
    }


};


using namespace json11;
using namespace std;

struct CaptionResult {

    int index = 0;
    bool final = false;
    double stability = 0.0;
    string caption_text;
    vector<string> output_lines;
    std::shared_ptr<CaptionResult> last_final_result;
    std::chrono::steady_clock::time_point created_at;

    CaptionResult(
            int index,
            bool final,
            double stability,
            string caption_text
    ) :
            index(index),
            final(final),
            stability(stability),
            caption_text(caption_text),
            created_at(std::chrono::steady_clock::now()) {
    }
};

class CaptionResultHandler {
    CaptionFormatSettings settings;

    std::shared_ptr<CaptionResult> last_final_result;
    string last_line;

public:
    explicit CaptionResultHandler(CaptionFormatSettings settings);

    shared_ptr<CaptionResult> parse_caption_object(const string &caption_obj, bool fillup_with_previous);

    void clear_history();
};

#endif //CPPTESTING_CAPTIONRESULTHANDLER_H
