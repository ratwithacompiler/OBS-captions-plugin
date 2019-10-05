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


#ifndef UTILS_H
#define UTILS_H

#include <cstring>
#include <cstdlib>

#include <sstream>
#include <vector>
#include <iostream>
#include <algorithm>

static std::string random_string(const int count) {
    std::string output;
    static const char alphanum[] =
            "0123456789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz";

    for (int i = 0; i < count; ++i) {
        output.push_back(alphanum[rand() % (sizeof(alphanum) - 1)]);
    }

    return output;
}


static int word_filter(const string &input, string &output, const vector<string> &banned_words_lowercase) {
    if (input.empty())
        return 0;

    int removed_cnt = 0;
    istringstream stream(input);
    string word;
    while (getline(stream, word, ' ')) {
//        cout << "word '" << word << "'" << endl;

        if (word.empty()) {
            output.append(" ");
            continue;
        }

        string lower_word(word);
        std::transform(lower_word.begin(), lower_word.end(), lower_word.begin(), ::tolower);

        if (std::find(banned_words_lowercase.begin(), banned_words_lowercase.end(), lower_word) != banned_words_lowercase.end()) {
            removed_cnt++;
//            cout << "AHH banned word '" << word << "'" << endl;
            continue;
        }

        output.append(word);
        if (!stream.eof())
            output.append(" ");
    }
    return removed_cnt;
}


#endif // UTILS_H