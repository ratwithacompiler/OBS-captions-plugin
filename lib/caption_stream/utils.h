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

#include <random>

static std::mutex wp_rnd_mut;
static std::unique_ptr<std::mt19937> wp_rnd_ins;

static void wp_rnd_init() {
    if (wp_rnd_ins)
        return;

    try {
        std::random_device dev;
        wp_rnd_ins = std::make_unique<std::mt19937>(dev());
    } catch (...) {
        printf("wp_rnd_init fail\n");
    }
}

static unsigned int wp_rnd_num() {
    if (wp_rnd_ins)
        return wp_rnd_ins->operator()();
    return rand();
}

static std::string random_string(const int count) {
    std::lock_guard<mutex> lock(wp_rnd_mut);
    wp_rnd_init();

    std::string output;
    static const char alphanum[] =
            "0123456789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz";

    for (int i = 0; i < count; ++i) {
        output.push_back(alphanum[wp_rnd_num() % (sizeof(alphanum) - 1)]);
    }

    return output;
}


#endif // UTILS_H