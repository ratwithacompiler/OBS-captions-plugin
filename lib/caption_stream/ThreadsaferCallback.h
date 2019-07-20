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
