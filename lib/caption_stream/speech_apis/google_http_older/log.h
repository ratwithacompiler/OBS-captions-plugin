/*
 * Copyright (c) 2012 David Rodrigues
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef __MACROLOGGER_H__
#define __MACROLOGGER_H__

#ifdef __OBJC__
#import <Foundation/Foundation.h>
#else

#include <time.h>
#include <string.h>

#endif

// === auxiliar functions
static inline char *timenow();

#define _FILE strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__

#define NO_LOG          0x00
#define ERROR_LEVEL     0x01
#define INFO_LEVEL      0x02
#define DEBUG_LEVEL     0x03

#ifndef LOG_LEVEL
#define LOG_LEVEL   DEBUG_LEVEL
#endif

#ifdef __OBJC__

#if __has_feature(objc_arc)
#define AUTORELEASEPOOL_BEGIN   @autoreleasepool {
#define AUTORELEASEPOOL_END     }
#define RELEASE(OBJ)            OBJ = nil
#else
#define AUTORELEASEPOOL_BEGIN   NSAutoreleasePool *_pool = [[NSAutoreleasePool alloc] init];
#define AUTORELEASEPOOL_END     [_pool release];
#define RELEASE(OBJ)            [OBJ release];
#endif

#define PRINTFUNCTION(format, ...)      objc_print(@format, __VA_ARGS__)
#elif _WIN32
#define PRINTFUNCTION(format, ...)      printf(format"\n", __VA_ARGS__)
#else
#define PRINTFUNCTION(format, ...)      fprintf(stderr, format, __VA_ARGS__)
#endif

#define LOG_FMT             "%s | %-7s | %-15s | %s:%d | "
#define LOG_ARGS(LOG_TAG)   timenow(), LOG_TAG, _FILE, __FUNCTION__, __LINE__

#define NEWLINE     "\n"

#define ERROR_TAG   "ERROR"
#define INFO_TAG    "INFO"
#define DEBUG_TAG   "DEBUG"

#if LOG_LEVEL >= DEBUG_LEVEL
#define debug_log(message, ...)     PRINTFUNCTION(LOG_FMT message NEWLINE, LOG_ARGS(DEBUG_TAG), ##__VA_ARGS__)
#else
#define debug_log(message, ...)
#endif

#if LOG_LEVEL >= INFO_LEVEL
#define info_log(message, ...)      PRINTFUNCTION(LOG_FMT message NEWLINE, LOG_ARGS(INFO_TAG), ##__VA_ARGS__)
#else
#define info_log(message, ...)
#endif

#if LOG_LEVEL >= ERROR_LEVEL
#define error_log(message, ...)     PRINTFUNCTION(LOG_FMT message NEWLINE, LOG_ARGS(ERROR_TAG), ##__VA_ARGS__)
#else
#define error_log(message, ...)
#endif

#if LOG_LEVEL >= NO_LOGS
#define LOG_IF_ERROR(condition, message, ...) if (condition) PRINTFUNCTION(LOG_FMT message NEWLINE, LOG_ARGS(ERROR_TAG), ##__VA_ARGS__)
#else
#define LOG_IF_ERROR(condition, message, ...)
#endif

static inline char *timenow() {
    static char buffer[64];
    time_t rawtime;
    struct tm *timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(buffer, 64, "%Y-%m-%d %H:%M:%S", timeinfo);

    return buffer;
}

#ifdef __OBJC__

static inline void objc_print(NSString *format, ...) {
    AUTORELEASEPOOL_BEGIN
    va_list args;
    va_start(args, format);
    NSString *logStr = [[NSString alloc] initWithFormat:format arguments:args];
    fprintf(stderr, "%s", [logStr UTF8String]);
    RELEASE(logStr);
    va_end(args);
    AUTORELEASEPOOL_END
}

#endif

#endif
