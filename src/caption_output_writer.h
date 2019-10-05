//
// Created by Rat on 31.08.19.
//

#ifndef OBS_GOOGLE_CAPTION_PLUGIN_CAPTION_OUTPUT_WRITER_H
#define OBS_GOOGLE_CAPTION_PLUGIN_CAPTION_OUTPUT_WRITER_H

#include "thirdparty/cameron314/blockingconcurrentqueue.h"
#include "log.c"


using steady_time_point = chrono::steady_clock::time_point;

struct CaptionOutput {
    shared_ptr<OutputCaptionResult> output_result;
    bool interrupted;
    bool is_clearance;

    CaptionOutput(shared_ptr<OutputCaptionResult> output_result, bool interrupted, bool is_clearance) :
            output_result(output_result),
            interrupted(interrupted),
            is_clearance(is_clearance) {};

    CaptionOutput() :
            interrupted(false),
            is_clearance(false) {}
};

struct CaptionOutputControl {
    moodycamel::BlockingConcurrentQueue<CaptionOutput> caption_queue;
    volatile bool stop = false;

    void stop_soon() {
        debug_log("CaptionOutputControl stop_soon()");
        stop = true;
        caption_queue.enqueue(CaptionOutput());
    }

    ~CaptionOutputControl() {
        debug_log("~CaptionOutputControl");
    }
};

static void caption_output_writer_loop(shared_ptr<CaptionOutputControl> control, bool to_stream) {
    // TODO: minimum_time_between_captions arg to optionally hold next caption if still too soon after previous one
    // just skip in between ones, maybe option to fall behind instead as well?

    string to_what(to_stream ? "streaming" : "recording");
    info_log("caption_output_writer_loop %s starting", to_what.c_str());

    string previous_line;
    CaptionOutput caption_output;
    int active_delay_sec;
    bool got_item;
    obs_output_t *output = nullptr;


    double waited_left_secs = 0;
    while (!control->stop) {
        control->caption_queue.wait_dequeue(caption_output);

        if (control->stop)
            break;

        if (!caption_output.output_result) {
            info_log("got empty CaptionOutput.output_result???");
            continue;
        }

        if (to_stream)
            output = obs_frontend_get_streaming_output();
        else
            output = obs_frontend_get_recording_output();

        if (!output) {
            info_log("built caption lines, no output, not sending, not %s?: '%s'", to_what.c_str(),
                     caption_output.output_result->output_line.c_str());
            continue;
        }

        if (!caption_output.is_clearance and caption_output.output_result->output_line.empty()) {
            debug_log("ingoring empty non clearance, %s", to_what.c_str());
            continue;
        }

        if (caption_output.output_result->output_line == previous_line) {
//            debug_log("ignoring duplicate %s line: %s", to_what.c_str(), previous_line.c_str());
            continue;
        }
        previous_line = caption_output.output_result->output_line;

        waited_left_secs = 0;

        active_delay_sec = obs_output_get_active_delay(output);
        if (active_delay_sec) {
            auto since_creation = chrono::steady_clock::now() - caption_output.output_result->caption_result.created_at;
            chrono::seconds wanted_delay(active_delay_sec);

//            debug_log("since_creation %f", chrono::duration_cast<std::chrono::duration<double >>(since_creation).count());
//            debug_log("wanted_delay %f", chrono::duration_cast<std::chrono::duration<double >>(wanted_delay).count());

            auto wait_left = wanted_delay - since_creation;
            if (wait_left > wanted_delay) {
                info_log("capping delay, wtf, negative duration?, got %f, max %f",
                         chrono::duration_cast<std::chrono::duration<double >>(wait_left).count(),
                         chrono::duration_cast<std::chrono::duration<double >>(wanted_delay).count()
                );
                wait_left = wanted_delay;
            }

            waited_left_secs = chrono::duration_cast<std::chrono::duration<double >>(wait_left).count();
            debug_log("caption_output_writer_loop %s sleeping for %f seconds",
                      to_what.c_str(), waited_left_secs);

            std::this_thread::sleep_for(wait_left);

            if (control->stop)
                break;
        }

        debug_log("sending caption %s line now, waited %f: '%s'",
                  to_what.c_str(), waited_left_secs, caption_output.output_result->output_line.c_str());

        obs_output_output_caption_text2(output, caption_output.output_result->output_line.c_str(), 0.0);
        obs_output_release(output);
    }

    info_log("caption_output_writer_loop %s done", to_what.c_str());
}

#endif //OBS_GOOGLE_CAPTION_PLUGIN_CAPTION_OUTPUT_WRITER_H
