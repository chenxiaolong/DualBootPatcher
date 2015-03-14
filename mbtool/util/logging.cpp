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
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef USE_ANDROID_LOG
#include <android/log.h>
#endif

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

StdioLogger::StdioLogger(std::FILE *stream) : _stream(stream)
{
}

void StdioLogger::log(LogLevel prio, const std::string &msg)
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
    fprintf(_stream, "%s %s\n", stdprio, msg.c_str());
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

void KmsgLogger::log(LogLevel prio, const std::string &msg)
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

    snprintf(_buf, KMSG_BUF_SIZE, "%s" LOG_TAG ": %s", kprio, msg.c_str());
    write(_fd, _buf, strlen(_buf));
}


#ifdef USE_ANDROID_LOG
void AndroidLogger::log(LogLevel prio, const std::string &msg)
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

    __android_log_print(logcatprio, LOG_TAG, "%s", msg.c_str());
}
#endif


static std::shared_ptr<BaseLogger> logger;

void log_set_logger(std::shared_ptr<BaseLogger> logger_local)
{
    logger = std::move(logger_local);
}

void log(LogLevel prio, const std::string &msg)
{
    if (!logger) {
        logger = std::make_shared<StdioLogger>(stdout);
    }

    logger->log(prio, msg);
}

}
}