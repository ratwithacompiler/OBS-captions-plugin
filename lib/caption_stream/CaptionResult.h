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
#ifndef OBS_GOOGLE_CAPTION_PLUGIN_CAPTIONRESULT_H
#define OBS_GOOGLE_CAPTION_PLUGIN_CAPTIONRESULT_H



#include <json11.hpp>
using namespace json11;


using namespace std;

struct CaptionResult {

    int index = 0;
    bool final = false;
    double stability = 0.0;
    string caption_text;
    string raw_message;

    std::chrono::steady_clock::time_point created_at;

    CaptionResult(
            int index,
            bool final,
            double stability,
            string caption_text,
            string raw_message
    ) :
            index(index),
            final(final),
            stability(stability),
            caption_text(caption_text),
            raw_message(raw_message),
            created_at(std::chrono::steady_clock::now()) {
    }
};

static CaptionResult* parse_caption_obj(const string &msg_obj) {
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
    return new CaptionResult(result_index, is_final, highest_stability, caption_text, msg_obj);
}







#endif //OBS_GOOGLE_CAPTION_PLUGIN_CAPTIONRESULT_H
