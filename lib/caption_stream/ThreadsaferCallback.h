//
// Created by Rat on 2019-06-13.
//

#ifndef OBS_GOOGLE_CAPTION_PLUGIN_THREADSAFERCALLBACK_H
#define OBS_GOOGLE_CAPTION_PLUGIN_THREADSAFERCALLBACK_H

#include <thread>
#include <mutex>

template<typename T>
class ThreadsaferCallback {
public:
    T callback_fn;
    bool lock_to_clear = true;
    std::recursive_mutex mutex;
//    std::function<T> callback_fn;

    ThreadsaferCallback() {};

    ThreadsaferCallback(
//            std::function<T> callback_fn,
            T callback_fn,
            bool lock_to_clear = true
    ) :
            callback_fn(callback_fn),
            lock_to_clear(lock_to_clear) {}


    void clear() {
        if (lock_to_clear) {
            std::lock_guard<std::recursive_mutex> lock(mutex);
            callback_fn = nullptr;
        } else
            callback_fn = nullptr;
    }

//    void set(std::function<T> new_callback_fn) {
    void set(T new_callback_fn, bool lock_to_clear = true) {
        clear();
        {
            std::lock_guard<std::recursive_mutex> lock(mutex);
            this->callback_fn = new_callback_fn;
            this->lock_to_clear = lock_to_clear;
        }
    }

    ~ThreadsaferCallback() {
        clear();
    }
};


#endif //OBS_GOOGLE_CAPTION_PLUGIN_THREADSAFERCALLBACK_H
