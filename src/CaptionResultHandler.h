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
#include <CaptionStream.h>

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
    }

    void print(const char *line_prefix = "") {
        printf("%sCaptionFormatSettings\n", line_prefix);
        printf("%s  caption_line_length: %d\n", line_prefix, caption_line_length);
        printf("%s  caption_line_count: %d\n", line_prefix, caption_line_count);
        printf("%s  caption_insert_newlines: %d\n", line_prefix, caption_insert_newlines);
        printf("%s  manual_banned_words: %lu\n", line_prefix, manual_banned_words.size());
        for (auto &word : manual_banned_words)
            printf("%s        '%s'\n", line_prefix, word.c_str());

//        printf("%s-----------\n", line_prefix);
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


struct OutputCaptionResult {

    CaptionResult caption_result;
    string clean_caption_text;

    vector<string> output_lines; //optionally cleaned and filled with history
    string output_line; // joined output_lines

    OutputCaptionResult(
            const CaptionResult &caption_result
    ) : caption_result(caption_result) {

    }
};


class CaptionResultHandler {
    CaptionFormatSettings settings;

//    std::shared_ptr<CaptionResult> last_final_result;
//    string last_line;

public:
    explicit CaptionResultHandler(CaptionFormatSettings settings);

    shared_ptr<OutputCaptionResult> prepare_caption_output(
            const CaptionResult &caption_result,
            bool fillup_with_previous,
            bool insert_newlines,
            const std::vector<std::shared_ptr<OutputCaptionResult>> &result_history);

};

#endif //CPPTESTING_CAPTIONRESULTHANDLER_H
