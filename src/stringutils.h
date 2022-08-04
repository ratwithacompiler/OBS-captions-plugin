#ifndef OBS_GOOGLE_CAPTION_PLUGIN_STRINGUTILS_H
#define OBS_GOOGLE_CAPTION_PLUGIN_STRINGUTILS_H

#include <QVector>
#include <QStringList>
#include <QTextBoundaryFinder>
#include <QRegularExpression>

static void string_capitalization(string &line, const CapitalizationType capitalization) {
    if (capitalization == CAPITALIZATION_ALL_CAPS)
        std::transform(line.begin(), line.end(), line.begin(), ::toupper);

    else if (capitalization == CAPITALIZATION_ALL_LOWERCASE)
        std::transform(line.begin(), line.end(), line.begin(), ::tolower);
}

static void lstrip(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
}

static bool isAscii(const std::string &text) {
    for (auto c: text) {
        if (static_cast<unsigned char>(c) > 127) {
            return false;
        }
    }
    return true;
}

static void splitSmallest(QVector<QString> &out_chars, const QString &word) {
    QTextBoundaryFinder test(QTextBoundaryFinder::Grapheme, word);
    int start = 0;
    while (test.toNextBoundary() != -1 && test.position() <= word.length()) {
        out_chars.push_back(word.mid(start, test.position() - start));
        start = test.position();
    }

    for (int i = 0; i < out_chars.length(); i++) {
    }
}

static void split_into_lines_unicode_ish(vector<string> &output_lines, const string &text, const uint max_line_length) {
    QString qtext = QString::fromStdString(text).simplified();

    QStringList words = qtext.split(QRegularExpression("\\s+"));
    uint cnt = 0;

    QString line;
    for (auto word: words) {

        int new_len = line.size() + (line.isEmpty() ? 0 : 1) + word.size();
        if (new_len <= max_line_length) {
            // still fits into line
            if (!line.isEmpty())
                line.append(" ");
            line.append(word);
        } else {
            if (word.length() > max_line_length) {
                // current word longer than single line, split

                if (!line.isEmpty()) {
                    if (line.length() + 2 <= max_line_length) {
                        // enough space for " " and more
                        line.append(" ");
                    } else {
                        // current line is full (or would be with added space),
                        // add it and clear
                        output_lines.push_back(line.toStdString());
                        line = "";
                    }
                }

                QVector<QString> gms;
                splitSmallest(gms, word);

                for (auto i: gms) {
                    if (line.length() + i.length() <= max_line_length) {
                        line.append(i);
                    } else {
                        output_lines.push_back(line.toStdString());
                        line = i;
                    }
                }
            } else {
                if (!line.isEmpty())
                    output_lines.push_back(line.toStdString());
                line = word;
            }
        }
    }

    if (!line.isEmpty())
        output_lines.push_back(line.toStdString());
}

static void split_into_lines_ascii(vector<string> &output_lines, const string &text, const uint max_line_length) {
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
            if (word.length() > max_line_length) {
                // current word longer than single line, split into multiple

                if (!line.empty()) {
                    if (line.length() + 2 <= max_line_length) {
                        // enough space for " " and more chars
                        line.append(" ");
                    } else {
                        // current line is full (or would be with added space),
                        // add it and clear
                        output_lines.push_back(line);
                        line = "";
                    }
                }

                for (auto i: word) {
                    if (line.length() < max_line_length) {
                        line.push_back(i);
                    } else {
                        output_lines.push_back(line);
                        line = i;
                    }
                }

            } else {
                if (!line.empty())
                    output_lines.push_back(line);

                line = word;
            }
        }
    }

    if (!line.empty())
        output_lines.push_back(line);
}


static void split_into_lines(vector<string> &output_lines, const string &text, const uint max_line_length) {
    // Split into multiple lines, none longer than [max_line_length].
    // Only splits a word if the word itself is longer than max_line_length.

    if (!isAscii(text)) {
        split_into_lines_unicode_ish(output_lines, text, max_line_length);
        return;
    }
    split_into_lines_ascii(output_lines, text, max_line_length);
}

static void join_strings(const vector<string> &lines, const string &joiner, string &output) {
    for (const string &a_line: lines) {
        if (!output.empty())
            output.append(joiner);

        output.append(a_line);
    }
}


#endif