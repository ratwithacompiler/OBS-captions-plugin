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

#include <grpcpp/grpcpp.h>
#include <google/cloud/speech/v1/cloud_speech.grpc.pb.h>

using google::cloud::speech::v1::RecognitionConfig;
using google::cloud::speech::v1::Speech;
using google::cloud::speech::v1::StreamingRecognizeRequest;
using google::cloud::speech::v1::StreamingRecognizeResponse;
using google::cloud::speech::v1::RecognitionConfig_AudioEncoding;

static void audio_sender_thread(std::shared_ptr<CaptionStream> self);

static void _audio_sender(CaptionStream &self);

static void read_results_loop_thread(
        CaptionStream &self,
        grpc::ClientReaderWriterInterface<StreamingRecognizeRequest, StreamingRecognizeResponse> *streamer
);

CaptionStream::CaptionStream(
        const CaptionStreamSettings settings
) :
        settings(settings),
        session_pair(random_string(15)) {
    debug_log("CaptionStream GRPC Speech, created session pair: %s", session_pair.c_str());
}

bool CaptionStream::start(std::shared_ptr<CaptionStream> self) {
    // Requires the CaptionStream to have been made as shared_pointer and passed to itself to start.
    // Kept by each thread to ensure the object isn't deconstructed before all threads are done.
    // I'm sure there are much nicer ways to do this but I don't know any of them so here we are.

    if (self.get() != this)
        return false;

    if (started)
        return false;

    started = true;
    thread upstream_thread(audio_sender_thread, self);
    upstream_thread.detach();
    return true;
}


static void audio_sender_thread(std::shared_ptr<CaptionStream> self) {
    debug_log("starting audio_sender_thread() thread");
    if (!self) {
        self->stop();
        debug_log("audio_sender_thread() no self");
        return;
    }

    _audio_sender(*self);
    self->stop();
    debug_log("finished audio_sender_thread() thread");
}

static bool write_config(
        grpc::ClientReaderWriterInterface<StreamingRecognizeRequest, StreamingRecognizeResponse> *streamer,
        const CaptionStreamSettings &settings
) {
    StreamingRecognizeRequest request;
    auto *streaming_config = request.mutable_streaming_config();
    streaming_config->set_interim_results(true);
    auto *rec_config = streaming_config->mutable_config();
    rec_config->set_encoding(RecognitionConfig_AudioEncoding::RecognitionConfig_AudioEncoding_LINEAR16);
    rec_config->set_sample_rate_hertz(16000);
    rec_config->set_language_code(settings.language);
    rec_config->set_profanity_filter(bool(settings.profanity_filter));
    rec_config->set_max_alternatives(0);


    return streamer->Write(request);
}

static void write_audio_loop(
        grpc::ClientReaderWriterInterface<StreamingRecognizeRequest, StreamingRecognizeResponse> *streamer,
        CaptionStream &self
) {
    uint chunk_count = 0;
    StreamingRecognizeRequest request;

    while (!self.is_stopped()) {
        string *audio_chunk = self.dequeue_audio_data(self.settings.send_timeout_ms * 1000);
        if (audio_chunk == nullptr) {
            debug_log("couldn't deque audio chunk in time");
            break;
        }

        if (audio_chunk->empty()) {
            debug_log("got 0 size audio chunk. ignoring");
            delete audio_chunk;
            continue;
        }

//        debug_log("qs %zu", audio_queue.size_approx());
//        std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 30));
//        std::this_thread::sleep_for(std::chrono::milliseconds(30));

        request.set_audio_content(*audio_chunk);
        if (!streamer->Write(request)) {
            debug_log("write_audio_loop write failed, stopping");
            delete audio_chunk;
            break;
        }
        if (chunk_count % 20 == 0)
            debug_log("sent audio chunk %d, %lu bytes", chunk_count, audio_chunk->size());
//        debug_log("sent audio chunk %d, %lu bytes", chunk_count, audio_chunk->size());

        delete audio_chunk;
        chunk_count++;
    }

}

static void read_results_loop_thread(
        CaptionStream &self,
        grpc::ClientReaderWriterInterface<StreamingRecognizeRequest, StreamingRecognizeResponse> *streamer
) {

    debug_log("read_results_loop_thread starting");
    StreamingRecognizeResponse response;
    std::chrono::steady_clock::time_point first_received_at;
    bool update_first_received_at = true;
    while (streamer->Read(&response)) {

        if (self.is_stopped())
            break;

        std::cout << "Result size: " << response.results_size() << std::endl;
        for (int r = 0; r < response.results_size(); ++r) {
            auto result = response.results(r);
            std::cout << "ind: " << r << " Result stability: " << result.stability() << " final: " << result.is_final() << std::endl;

            for (int a = 0; a < result.alternatives_size(); ++a) {
                auto alternative = result.alternatives(a);
                std::cout << "conf: " << alternative.confidence() << "; " << alternative.transcript() << std::endl;

                auto now = std::chrono::steady_clock::now();
                if (update_first_received_at)
                    first_received_at = now;

                CaptionResult cap_result(0, result.is_final(), result.stability(), alternative.transcript(), "", first_received_at, now);
                update_first_received_at = result.is_final();
                {
                    std::lock_guard<recursive_mutex> lock(self.on_caption_cb_handle.mutex);
                    if (self.on_caption_cb_handle.callback_fn) {
                        self.on_caption_cb_handle.callback_fn(cap_result);
                    }
                }
                break;

            }
            break;
        }
    }
    debug_log("read_results_loop_thread done");
    self.stop();
};

#ifdef GRPC_USE_INCLUDED_CERTS

#include "certs/roots.h"

#endif

static void _audio_sender(CaptionStream &self) {
    debug_log("_audio_sender");

    try {
        auto options = grpc::SslCredentialsOptions();

#ifdef GRPC_USE_INCLUDED_CERTS
        debug_log("using embedded certs");
        // grpc needs CA certs on Windows (and Unix depending on how it's been built)
        // just include as string directly, more convenient than shipping the roots.pem file
        std::string certs(ROOTS_PEM, ROOTS_PEM + sizeof(ROOTS_PEM) / sizeof(ROOTS_PEM[0]));
        options.pem_root_certs = certs;
#endif
        auto creds = grpc::SslCredentials(options);
        auto channel = grpc::CreateChannel("speech.googleapis.com", creds);
        std::unique_ptr<Speech::Stub> speech(Speech::NewStub(channel));

        grpc::ClientContext context;
        context.AddMetadata("x-goog-api-key", self.settings.api_key);
//        context.set_deadline()

        auto streamer = speech->StreamingRecognize(&context);

        debug_log("write speech config");
        if (write_config(streamer.get(), self.settings)) {
            debug_log("write speech config done");

            std::thread downstream_thread(&read_results_loop_thread, std::ref(self), streamer.get());

            debug_log("starting audio writes");
            write_audio_loop(streamer.get(), self);
            streamer->WritesDone();
            debug_log("audio writing finished");

            debug_log("waiting for downstream thread finish");
            downstream_thread.join();
            debug_log("downstream thread finished!");

            // read remaining otherwise Finish() can block?
            StreamingRecognizeResponse response;
            while (streamer->Read(&response))
                continue;
        } else {
            debug_log("write speech config failed");
        }

        auto status = streamer->Finish();
        if (!status.ok()) {
            error_log("grpc stream error: %s", status.error_message().c_str());
        }
    }
    catch (const std::exception &ex) {
        error_log("_audio_sender exception catch %s", ex.what());
    } catch (const std::string &ex) {
        error_log("_audio_sender exception  string %s", ex.c_str());
    } catch (...) {
        error_log("_audio_sender exception any error");
    }
    debug_log("_audio_sender done");
}

bool CaptionStream::is_stopped() {
    return stopped;
}

bool CaptionStream::queue_audio_data(const char *audio_data, const uint data_size) {
    if (is_stopped())
        return false;

    string *str = new string(audio_data, data_size);

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
    debug_log("CaptionStream stop()");
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
    debug_log("~CaptionStream deconstructor, deleted left %d in queue", cleared);

}
