//
// Created by Rat on 31.08.19.
//

#ifndef OBS_GOOGLE_CAPTION_PLUGIN_CAPTION_OUTPUT_WRITER_H
#define OBS_GOOGLE_CAPTION_PLUGIN_CAPTION_OUTPUT_WRITER_H

#include "thirdparty/cameron314/blockingconcurrentqueue.h"
#include "log.c"


using steady_time_point = chrono::steady_clock::time_point;

struct CaptionOutput {
    string line;
    steady_time_point created_at;

    CaptionOutput() : created_at(chrono::steady_clock::now()) {};

    CaptionOutput(const string &line, const steady_time_point &createdAt) : line(line), created_at(createdAt) {}
};

struct CaptionOutputControl {
    moodycamel::BlockingConcurrentQueue<CaptionOutput> caption_queue;
    volatile bool stop = false;

    void stop_soon() {
        info_log("CaptionOutputControl stop_soon()");
        stop = true;
        caption_queue.enqueue(CaptionOutput());
    }
};

static void caption_output_writer_loop(CaptionOutputControl *control, bool delete_control) {
    // TODO: minimum_time_between_captions arg to optionally hold next caption if still too soon after previous one
    // just skip in between ones, maybe option to fall behind instead as well?

    info_log("caption_output_writer_loop starting");

    CaptionOutput caption;
    int active_delay_sec;
    bool got_item;

    while (!control->stop) {
        control->caption_queue.wait_dequeue(caption);

        if (control->stop)
            break;

        obs_output_t *output = obs_frontend_get_streaming_output();
        if (!output) {
            info_log("built caption lines, no output, not sending, not live?: '%s'", caption.line.c_str());
            continue;
        }

        active_delay_sec = obs_output_get_active_delay(output);
        if (active_delay_sec) {
            auto since_creation = chrono::steady_clock::now() - caption.created_at;
            chrono::seconds wanted_delay(active_delay_sec);

            info_log("since_creation %f", chrono::duration_cast<std::chrono::duration<double >>(since_creation).count());
            info_log("wanted_delay %f", chrono::duration_cast<std::chrono::duration<double >>(wanted_delay).count());

            auto wait_left = wanted_delay - since_creation;
            if (wait_left > wanted_delay) {
                info_log("capping delay, wtf, negative duration?, got %f, max %f",
                         chrono::duration_cast<std::chrono::duration<double >>(wait_left).count(),
                         chrono::duration_cast<std::chrono::duration<double >>(wanted_delay).count()
                );
                wait_left = wanted_delay;
            }
            info_log("caption_output_writer_loop sleeping for %f seconds",
                     chrono::duration_cast<std::chrono::duration<double >>(wait_left).count());

            std::this_thread::sleep_for(wait_left);

            if (control->stop)
                break;
        }

        debug_log("sending caption line now: '%s'", caption.line.c_str());
        obs_output_output_caption_text2(output, caption.line.c_str(), 0.0);
        obs_output_release(output);
    }

    if (delete_control)
        delete control;

    info_log("caption_output_writer_loop done");
}

#endif //OBS_GOOGLE_CAPTION_PLUGIN_CAPTION_OUTPUT_WRITER_H
