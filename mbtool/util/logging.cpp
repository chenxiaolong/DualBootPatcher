/*
 * Copyright (C) 2014  Andrew Gunnerson <andrewgunnerson@gmail.com>
 *
 * This file is part of MultiBootPatcher
 *
 * MultiBootPatcher is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MultiBootPatcher is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MultiBootPatcher.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "util/logging.h"

#include <cstdio>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef USE_ANDROID_LOG
#include <android/log.h>
#endif

#include "util/string.h"

#define LOG_TAG "mbtool"

namespace mb
{
namespace util
{

#define STDLOG_LEVEL_ERROR   "[E]"
#define STDLOG_LEVEL_WARNING "[W]"
#define STDLOG_LEVEL_INFO    "[I]"
#define STDLOG_LEVEL_DEBUG   "[D]"
#define STDLOG_LEVEL_VERBOSE "[V]"

StdioLogger::StdioLogger(std::FILE *stream, bool show_timestamps)
    : _stream(stream), _show_timestamps(show_timestamps)
{
}

void StdioLogger::log(LogLevel prio, const char *fmt, va_list ap)
{
    if (!_stream) {
        return;
    }

    const char *stdprio = "";

    switch (prio) {
    case LogLevel::ERROR:
        stdprio = STDLOG_LEVEL_ERROR;
        break;
    case LogLevel::WARNING:
        stdprio = STDLOG_LEVEL_WARNING;
        break;
    case LogLevel::INFO:
        stdprio = STDLOG_LEVEL_INFO;
        break;
    case LogLevel::DEBUG:
        stdprio = STDLOG_LEVEL_DEBUG;
        break;
    case LogLevel::VERBOSE:
        stdprio = STDLOG_LEVEL_VERBOSE;
        break;
    }

    if (_show_timestamps) {
        struct timespec res;
        struct tm tm;
        clock_gettime(CLOCK_REALTIME, &res);
        localtime_r(&res.tv_sec, &tm);

        char buf[100];
        strftime(buf, sizeof(buf), "%Y/%m/%d %H:%M:%S %Z", &tm);
        fprintf(_stream, "[%s]", buf);
    }

    fprintf(_stream, "%s ", stdprio);
    vfprintf(_stream, fmt, ap);
    fprintf(_stream, "\n");
    fflush(_stream);
}


#define KMSG_LEVEL_DEBUG    "<7>"
#define KMSG_LEVEL_INFO     "<6>"
#define KMSG_LEVEL_NOTICE   "<5>"
#define KMSG_LEVEL_WARNING  "<4>"
#define KMSG_LEVEL_ERROR    "<3>"
#define KMSG_LEVEL_CRITICAL "<2>"
#define KMSG_LEVEL_ALERT    "<1>"
#define KMSG_LEVEL_EMERG    "<0>"
#define KMSG_LEVEL_DEFAULT  "<d>"

KmsgLogger::KmsgLogger()
{
    static int open_mode = O_WRONLY | O_NOCTTY | O_CLOEXEC;
    static const char *kmsg = "/dev/kmsg";
    static const char *kmsg2 = "/dev/kmsg.mbtool";

    _fd = open(kmsg, open_mode);
    if (_fd < 0) {
        // If /dev/kmsg hasn't been created yet, then create our own
        // Character device mode: S_IFCHR | 0600
        if (mknod(kmsg2, S_IFCHR | 0600, makedev(1, 11)) == 0) {
            _fd = open(kmsg2, open_mode);
            if (_fd >= 0) {
                unlink(kmsg2);
            }
        }
    }
}

KmsgLogger::~KmsgLogger()
{
    if (_fd > 0) {
        close(_fd);
    }
}

void KmsgLogger::log(LogLevel prio, const char *fmt, va_list ap)
{
    if (_fd < 0) {
        return;
    }

    const char *kprio = KMSG_LEVEL_DEFAULT;

    switch (prio) {
    case LogLevel::ERROR:
        kprio = KMSG_LEVEL_ERROR;
        break;
    case LogLevel::WARNING:
        kprio = KMSG_LEVEL_WARNING;
        break;
    case LogLevel::INFO:
        kprio = KMSG_LEVEL_INFO;
        break;
    case LogLevel::DEBUG:
        kprio = KMSG_LEVEL_DEBUG;
        break;
    case LogLevel::VERBOSE:
        kprio = KMSG_LEVEL_DEFAULT;
        break;
    }

    std::string new_fmt = format("%s" LOG_TAG ": %s\n", kprio, fmt);
    std::size_t len = vsnprintf(_buf, KMSG_BUF_SIZE, new_fmt.c_str(), ap);

    // Make user aware of any truncation
    if (len >= KMSG_BUF_SIZE) {
        static const char trunc[] = " [trunc...]\n";
        memcpy(_buf + sizeof(_buf) - sizeof(trunc), trunc, sizeof(trunc));
    }

    write(_fd, _buf, strlen(_buf));
    //vdprintf(_fd, new_fmt.c_str(), ap);
}


#ifdef USE_ANDROID_LOG
void AndroidLogger::log(LogLevel prio, const char *fmt, va_list ap)
{
    int logcatprio;

    switch (prio) {
    case LogLevel::ERROR:
        logcatprio = ANDROID_LOG_ERROR;
        break;
    case LogLevel::WARNING:
        logcatprio = ANDROID_LOG_WARN;
        break;
    case LogLevel::INFO:
        logcatprio = ANDROID_LOG_INFO;
        break;
    case LogLevel::DEBUG:
        logcatprio = ANDROID_LOG_DEBUG;
        break;
    case LogLevel::VERBOSE:
        logcatprio = ANDROID_LOG_VERBOSE;
        break;
    }

    __android_log_vprint(logcatprio, LOG_TAG, fmt, ap);
}
#endif


static std::shared_ptr<BaseLogger> logger;

void log_set_logger(std::shared_ptr<BaseLogger> logger_local)
{
    logger = std::move(logger_local);
}

void log(LogLevel prio, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    logv(prio, fmt, ap);

    va_end(ap);
}

void logv(LogLevel prio, const char *fmt, va_list ap)
{
    int saved_errno = errno;

    if (!logger) {
        logger = std::make_shared<StdioLogger>(stdout, false);
    }

    logger->log(prio, fmt, ap);

    errno = saved_errno;
}

}
}
