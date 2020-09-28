/******************************************************************************
Copyright (C) 2020 by <rat.with.a.compiler@gmail.com>

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

#ifndef OBS_GOOGLE_CAPTION_PLUGIN_WORDREPLACER_H
#define OBS_GOOGLE_CAPTION_PLUGIN_WORDREPLACER_H

#include <string>
#include <vector>
#include <regex>
#include <QString>

using namespace std;

class WordReplacement {
private:
    string type;
    string from;
    string to;

public:
    WordReplacement(const string &type, const string &from, const string &to) :
            type(type), from(from), to(to) {}

    bool operator==(const WordReplacement &rhs) const {
        return type == rhs.type &&
               from == rhs.from &&
               to == rhs.to;
    }

    bool operator!=(const WordReplacement &rhs) const {
        return !(rhs == *this);
    }

    const string &get_type() const {
        return type;
    }

    const string &get_from() const {
        return from;
    }

    const string &get_to() const {
        return to;
    }

    friend class Replacer;
};

class Rep {
private:
    bool use_text;
    std::regex reg;
    string reg_to;
    QString text_from;
    QString text_to;
    bool text_case_sensitive;
public:

    Rep(const regex &reg, const string &to) :
            use_text(false), reg(reg), reg_to(to) {}

    Rep(const string &from, const string &to, bool case_sensitive) :
            use_text(true), text_from(QString::fromStdString(from)), text_to(QString::fromStdString(to)),
            text_case_sensitive(case_sensitive) {}

    string replace(const string &input) const {
        if (use_text) {
            if (text_from.isEmpty())
                return input;

            if (text_case_sensitive)
                return QString::fromStdString(input).replace(text_from, text_to, Qt::CaseSensitive).toStdString();
            else
                return QString::fromStdString(input).replace(text_from, text_to, Qt::CaseInsensitive).toStdString();
        } else {
            return std::regex_replace(input, reg, reg_to);
        }
    }
};

class Replacer {
    std::vector<Rep> regs;

public:
    Replacer(const vector<WordReplacement> &replacements, bool ignore_invalid) {
        set_replacements(replacements, ignore_invalid);
    }

    bool has_replacements() const {
        return !regs.empty();
    }

    string replace(const string &input) const {
        string res(input);
        for (const auto &rep: regs) {
            res = rep.replace(res);
        }

        return res;
    }

private:
    void set_replacements(const std::vector<WordReplacement> &reps, bool ignore_invalid) {
        addReps(reps, ignore_invalid);
//        addRepsRegex(reps, ignore_invalid);
    }

    void addReps(const std::vector<WordReplacement> &reps, bool ignore_invalid) {
        for (WordReplacement rep: reps) {
            if (rep.from.empty())
                continue;

            try {
                if (rep.type == "text_case_sensitive") {
                    regs.push_back(Rep(rep.from, rep.to, true));
                } else if (rep.type == "text_case_insensitive") {
                    regs.push_back(Rep(rep.from, rep.to, false));
                } else if (rep.type == "regex_case_sensitive") {
                    regs.push_back(Rep(regex(rep.from), rep.to));
                } else if (rep.type == "regex_case_insensitive") {
                    regs.push_back(Rep(regex(rep.from, regex::icase), rep.to));
                } else {
                    throw string("invalid replacement type: " + rep.type);
                }
            }
            catch (exception) {
                if (!ignore_invalid)
                    throw;
            }
        }
    }

    void addRepsRegex(const std::vector<WordReplacement> &reps, bool ignore_invalid) {
        std::regex escape_reg(R"([/|[\]{}()\\^$*+?.])");

        for (WordReplacement rep: reps) {
            if (rep.from.empty())
                continue;

            try {
                if (rep.type == "text_case_sensitive") {
                    string escaped = std::regex_replace(rep.from, escape_reg, "\\$0");
                    regs.push_back(Rep(regex(escaped), rep.to));
                } else if (rep.type == "text_case_insensitive") {
                    string escaped = std::regex_replace(rep.from, escape_reg, "\\$0");
                    regs.push_back(Rep(regex(escaped, regex::icase), rep.to));
                } else if (rep.type == "regex_case_sensitive") {
                    regs.push_back(Rep(regex(rep.from), rep.to));
                } else if (rep.type == "regex_case_insensitive") {
                    regs.push_back(Rep(regex(rep.from, regex::icase), rep.to));
                } else {
                    throw string("invalid replacement type: " + rep.type);
                }
            }
            catch (exception) {
                if (!ignore_invalid)
                    throw;
            }
        }
    }


};

static std::vector<WordReplacement> wordRepsFromStrs(const string &type, const std::vector<string> &strings) {
    auto words = std::vector<WordReplacement>();

    for (auto from: strings) {
        if (from.empty())
            continue;

        words.push_back(WordReplacement(type, from, ""));
    }
    return words;
}


static vector<WordReplacement> combineWordReps(
        const vector<WordReplacement> &manualReplacements,
        const vector<WordReplacement> &defaultReplacements) {
    auto reps = vector<WordReplacement>();

    for (const auto &i: defaultReplacements)
        reps.push_back(i);

    for (const auto &i: manualReplacements)
        reps.push_back(i);

    return reps;
}


#endif //OBS_GOOGLE_CAPTION_PLUGIN_WORDREPLACER_H
