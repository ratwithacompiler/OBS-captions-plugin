#ifndef CAPTIONS_LOG_H
#define CAPTIONS_LOG_H

#pragma once

#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>

#define CAPTIONS_LOG_PREFIX "[captions_plugin] "

enum CaptionsLogLevel {
    CAPTIONS_LOG_ERROR = 0,
    CAPTIONS_LOG_WARNING = 1,
    CAPTIONS_LOG_INFO = 2,
    CAPTIONS_LOG_DEBUG = 3,
};

typedef void (*captions_log_sink_t)(int level, const char *line);

inline void captions_log_stderr_sink(int level, const char *line) {
    (void) level;
    fprintf(stderr, "%s\n", line);
}

inline std::atomic<captions_log_sink_t> captions_log_sink{captions_log_stderr_sink};
inline std::atomic<int> captions_log_level{CAPTIONS_LOG_DEBUG};

#if defined(__GNUC__) || defined(__clang__)
#define CAPTIONS_LOG_FORMAT_ATTR(fmt_index, args_index) __attribute__((__format__(__printf__, fmt_index, args_index)))
#else
#define CAPTIONS_LOG_FORMAT_ATTR(fmt_index, args_index)
#endif

inline const char *captions_log_basename(const char *path) {
    const char *last = path;
    for (const char *at = path; *at; at++)
        if (*at == '/' || *at == '\\')
            last = at + 1;

    return last;
}

CAPTIONS_LOG_FORMAT_ATTR(5, 6)
inline void captions_log(int level, const char *file, const char *func, int line, const char *format, ...) {
    if (level > captions_log_level.load(std::memory_order_relaxed))
        return;

    va_list args, args_retry;
    va_start(args, format);
    va_copy(args_retry, args);

    char stack_buf[1024];
    int needed = vsnprintf(stack_buf, sizeof(stack_buf), format, args);
    va_end(args);

    std::string message;
    if (needed < 0) {
        message = format;
    } else if ((size_t) needed < sizeof(stack_buf)) {
        message.assign(stack_buf, (size_t) needed);
    } else {
        message.resize((size_t) needed + 1);
        vsnprintf(&message[0], message.size(), format, args_retry);
        message.resize((size_t) needed);
    }
    va_end(args_retry);

    for (char &a_char: message)
        if (a_char == '\n' || a_char == '\r')
            a_char = ' ';

    std::string out(CAPTIONS_LOG_PREFIX);
    out.append(captions_log_basename(file)).append(" | ").append(func).append(":");
    out.append(std::to_string(line)).append(" | ").append(message);

    captions_log_sink.load(std::memory_order_relaxed)(level, out.c_str());
}

#define debug_log(format, ...) captions_log(CAPTIONS_LOG_DEBUG, __FILE__, __func__, __LINE__, format, ##__VA_ARGS__)
#define info_log(format, ...) captions_log(CAPTIONS_LOG_INFO, __FILE__, __func__, __LINE__, format, ##__VA_ARGS__)
#define warn_log(format, ...) captions_log(CAPTIONS_LOG_WARNING, __FILE__, __func__, __LINE__, format, ##__VA_ARGS__)
#define error_log(format, ...) captions_log(CAPTIONS_LOG_ERROR, __FILE__, __func__, __LINE__, format, ##__VA_ARGS__)

#endif //CAPTIONS_LOG_H
