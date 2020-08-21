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


#include <string>
#include <sstream>
#include "CaptionStream.h"
#include "utils.h"
#include "log.h"

//#define BUFFER_SIZE 1024
#define BUFFER_SIZE 4096

#include <json11.cpp>
using namespace json11;

static CaptionResult *parse_caption_obj(const string &msg_obj,
                                        const std::chrono::steady_clock::time_point &first_received_at,
                                        const std::chrono::steady_clock::time_point &received_at
) {
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
    return new CaptionResult(result_index, is_final, highest_stability, caption_text, msg_obj, first_received_at, received_at);
}


int read_until_contains(TcpConnection *connection, string &buffer, const char *required_contents) {
    int newline_pos = buffer.find(required_contents);
    if (newline_pos != string::npos)
        return newline_pos;

    char chunk[BUFFER_SIZE];
    do {
        int read_cnt = connection->receive_at_most(chunk, BUFFER_SIZE);
        if (read_cnt <= 0)
            return -1;

        buffer.append(chunk, read_cnt);
    } while ((newline_pos = buffer.find(required_contents)) == string::npos);
    return newline_pos;
}


CaptionStream::CaptionStream(
        CaptionStreamSettings settings
) : upstream(TcpConnection(GOOGLE, PORTUP)),
    downstream(TcpConnection(GOOGLE, PORTDOWN)),

    settings(settings),

    session_pair(random_string(15)),
    upstream_thread(nullptr),
    downstream_thread(nullptr) {

    debug_log("CaptionStream Google HTTP, created session pair: %s", session_pair.c_str());
}


bool CaptionStream::start(std::shared_ptr<CaptionStream> self) {
    // requires the CaptionStream to have been made as shared_pointer and passed to itself to start
    // kept by each thread to ensure object not deconstructed before threads done
    // I'm sure there are much nicer ways to do this but I don't know any of them so here we are

    if (self.get() != this)
        return false;

    if (started)
        return false;

    started = true;
    upstream_thread = new thread(&CaptionStream::upstream_run, this, self);
    return true;
}


void CaptionStream::upstream_run(std::shared_ptr<CaptionStream> self) {
    debug_log("starting upstream_run()");
    _upstream_run(self);
    stop();
    debug_log("finished upstream_run()");
}

void CaptionStream::downstream_run(std::shared_ptr<CaptionStream> self) {
    debug_log("starting downstream_run()");
    _downstream_run();
    stop();
    debug_log("finished downstream_run()");
}

void CaptionStream::_upstream_run(std::shared_ptr<CaptionStream> self) {
    try {
        upstream.connect(settings.connect_timeout_ms);

    } catch (ConnectError &ex) {
        debug_log("upstream connect error, %s", ex.what());
        return;
    }

    debug_log("upstream connected!");

    upstream.set_timeout(settings.send_timeout_ms);

    string post_req("POST /speech-api/full-duplex/v1/up?key=");
    post_req.append(settings.api_key);

    post_req.append("&pair=");
    post_req.append(session_pair);

    post_req.append("&lang=");
    post_req.append(settings.language);

    if (settings.profanity_filter)
        post_req.append("&pFilter=1");
    else
        post_req.append("&pFilter=0");

    post_req.append("&client=chromium&continuous&interim HTTP/1.1\r\n"
                    "Host: www.google.com\r\n"
                    "content-type: audio/l16; rate=16000\r\n"
                    "Accept: */\r\n"
                    "Accept-Encoding: gzip, deflat\r\n"
                    "User-Agent: TwitchStreamCaptioner ThanksGoogle\r\n"
                    "Transfer-Encoding: chunked\r\n"
                    "\r\n");

//    info_log("GOOGLE_API_KEY_STR: '%s'", GOOGLE_API_KEY_STR);
    if (!upstream.send_all(post_req.c_str(), post_req.size())) {
        error_log("upstream send head error");
        return;
    }

    if (is_stopped())
        return;

//    debug_log("post: %s", post_req.c_str());
    debug_log("sent head bytes %lu, language: %s, profanity filter: %d",
              post_req.size(), settings.language.c_str(), settings.profanity_filter);

    if (downstream_thread) {
        error_log("already has downstream thread, wtf");
        return;
    }

    downstream_thread = new thread(&CaptionStream::downstream_run, this, self);

    const string crlf("\r\n");
    uint chunk_count = 0;
    while (true) {
        if (is_stopped())
            return;

        string *audio_chunk = dequeue_audio_data(settings.send_timeout_ms * 1000);
        if (audio_chunk == nullptr) {
            error_log("couldn't deque audio chunk in time");
            return;
        }

//        info_log("qs %zu", audio_queue.size_approx());
//        std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 30));
//        std::this_thread::sleep_for(std::chrono::milliseconds(30));

        if (!audio_chunk->empty()) {

            std::stringstream stream;
            stream << std::hex << audio_chunk->size() << crlf << *audio_chunk << crlf;
            std::string request(stream.str());

            if (!upstream.send_all(request.c_str(), request.size())) {
                error_log("couldn't send audio chunk");
                delete audio_chunk;
                return;
            }

            if (chunk_count % 1000 == 0)
                debug_log("sent audio chunk %d, %lu bytes", chunk_count, audio_chunk->size());
//            debug_log("sent audio chunk %d, %lu bytes", chunk_count, audio_chunk->size());

        } else {
            error_log("got 0 size audio chunk. ignoring");
        }

        delete audio_chunk;
        chunk_count++;
    }
}


void CaptionStream::_downstream_run() {
    const uint crlf_len = 2;

    if (settings.download_thread_start_delay_ms) {
        debug_log("waiting %d ms to start download connection", settings.download_thread_start_delay_ms);
        std::this_thread::sleep_for(std::chrono::milliseconds(settings.download_thread_start_delay_ms));
    }

    try {
        downstream.connect(settings.connect_timeout_ms);
    } catch (ConnectError &ex) {
        error_log("downstream connect() error, %s", ex.what());
        return;
    }
    if (is_stopped())
        return;

    debug_log("downstream connected!");
    downstream.set_timeout(settings.send_timeout_ms);


    string get_req("GET /speech-api/full-duplex/v1/down?pair=");
    get_req.append(session_pair);
    get_req.append(" HTTP/1.1\r\n"
                   "Host: www.google.com\r\n"
                   "Accept: */*\r\n"
                   //                   "Accept-Encoding: gzip, deflate\r\n"
                   "User-Agent: TwitchStreamCaptioner ThanksGoogle\r\n"
                   "\r\n");

    if (!downstream.send_all(get_req.c_str(), get_req.size())) {
        error_log("downstream send head error");
        return;
    }

    if (is_stopped())
        return;

    debug_log("downstream header sent, %lu bytes!", get_req.size());
    downstream.set_timeout(settings.recv_timeout_ms);

    string head;
    // read until HTTP head end (or timeout)
    const int newlines_pos = read_until_contains(&downstream, head, "\r\n\r\n");
    if (newlines_pos == -1) {
        error_log("downstream read head error");
        return;
    }

    if (head.find("HTTP/1.1 200 OK") != 0) {
        error_log("downstream invalid response %s", head.c_str());
        return;
    }

    if (is_stopped())
        return;

//    debug_log("RESPONSE:\n%s\n", head.c_str());

    string rest = head.substr(newlines_pos + crlf_len + crlf_len, string::npos);
//    debug_log("rest: '%s'", rest.c_str());

    unsigned long chunk_length;


    int crlf_pos;
    std::chrono::steady_clock::time_point first_received_at;
    bool update_first_received_at = true;
    while (true) {
//        debug_log("rest: '%s'", rest.c_str());
        crlf_pos = read_until_contains(&downstream, rest, "\r\n");
        if (crlf_pos <= 0) {
            error_log("downstream read chunksize error, rest: %lu %s", rest.size(), rest.c_str());
            return;
        }

        if (is_stopped())
            return;

        try {
            chunk_length = std::stoul(rest.substr(0, crlf_pos), nullptr, 16);
//            if (!chunk_length) {
//                error_log("downstream parse chunksize = 0, wtf");
//                return;
//            }
        }
        catch (...) {
            error_log("downstream parse chunksize error, %s", rest.c_str());
            return;
        }

        uint chunk_data_start = crlf_pos + crlf_len;
        uint existing_chunk_byte_cnt = rest.size() - chunk_data_start;
        int needed_bytes = ((int) chunk_length) - existing_chunk_byte_cnt + crlf_len;

        if (needed_bytes > 0) {
//            debug_log("reading chunk, size: %lu, existing_chunk_byte_cnt: %d, needed %d",
//                      chunk_length, existing_chunk_byte_cnt, needed_bytes);

            int read = downstream.receive_at_least(rest, needed_bytes);
//            debug_log("hmm read: %d", read);
            if (read <= 0 || read < needed_bytes) {
                error_log("downstream read chunk data error");
                return;
            }
        }

        if (is_stopped())
            return;

        if (chunk_length) {
            string chunk_data = rest.substr(chunk_data_start, chunk_length);
            if (chunk_data.size() != chunk_length) {
                error_log("downstream read chunk data error, too few bytes, wtf, %lu %lu", chunk_data.size(), chunk_length);
                return;
            }


            try {
                auto now = std::chrono::steady_clock::now();
                if (update_first_received_at)
                    first_received_at = now;

                CaptionResult *result = parse_caption_obj(chunk_data, first_received_at, now);
                update_first_received_at = result->final;

                {
                    std::lock_guard<recursive_mutex> lock(on_caption_cb_handle.mutex);
                    if (on_caption_cb_handle.callback_fn) {
////                    debug_log("calling caption cb");
                        on_caption_cb_handle.callback_fn(*result);
                    }
                }

                delete result;

            } catch (string &ex) {
                info_log("couldn't parse caption message. Error: '%s'. Messsage: '%s'", ex.c_str(), chunk_data.c_str());
            }
            catch (...) {
                info_log("couldn't parse caption message. Messsage: '%s'", chunk_data.c_str());
            }

            if (is_stopped())
                return;
//            info_log("downstream chunk: %lu bytes, %s", chunk_data.size(), chunk_data.c_str());

        } else {
            info_log("ignoring zero data chunk");
        }
        rest.erase(0, chunk_data_start + chunk_length + 2); // also erase ending CRLF after data
    }
};

bool CaptionStream::is_stopped() {
    return stopped;
}

bool CaptionStream::queue_audio_data(const char *audio_data, const uint data_size) {
    if (is_stopped())
        return false;

    string *str = new string(audio_data, data_size);

    const int over_limit_cnt = audio_queue.size_approx() - settings.max_queue_depth;
    if (settings.max_queue_depth) {
        // rough estimate in dropping, dont care about being off by one or two in threaded cases

        int cleared_cnt = 0;
        while (audio_queue.size_approx() > settings.max_queue_depth) {
            string *item;
            if (audio_queue.try_dequeue(item)) {
                delete item;
                cleared_cnt++;
            }
        }
        if (cleared_cnt)
            debug_log("queue too big, dropped %d old items from queue %s", cleared_cnt, session_pair.c_str());
    }

//    debug_log("queued %s", session_pair.c_str());
    audio_queue.enqueue(str);
    return true;
}

string *CaptionStream::dequeue_audio_data(const std::int64_t timeout_us) {
    string *ret;
    if (audio_queue.wait_dequeue_timed(ret, timeout_us))
        return ret;

    return nullptr;
}


void CaptionStream::stop() {
    info_log("stop!!");
    on_caption_cb_handle.clear();
    stopped = true;

    string *to_unblock_uploader = new string();
    audio_queue.enqueue(to_unblock_uploader);
}


CaptionStream::~CaptionStream() {
    debug_log("~CaptionStream deconstructor");
    if (!is_stopped())
        stop();

    int cleared = 0;
    {
        string *item;
        while (audio_queue.try_dequeue(item)) {
            delete item;
            cleared++;
        }
    }
    debug_log("~CaptionStream deleting");


    if (upstream_thread) {
        upstream_thread->detach();
        delete upstream_thread;
        upstream_thread = nullptr;
    }

    if (downstream_thread) {
        downstream_thread->detach();
        delete downstream_thread;
        downstream_thread = nullptr;
    }

    debug_log("~CaptionStream deconstructor, deleted left %d in queue", cleared);

}

bool CaptionStream::is_started() {
    return started;
}


