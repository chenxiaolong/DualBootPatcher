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

#include "private/logging.h"

#include <unordered_map>

#define LOG_TAG "libmbp"


#if defined(__ANDROID__) && !defined(MBP_NO_LOGCAT)
#include <android/log.h>

static void logcat(mbp::LogLevel prio, const std::string &msg)
{
    int logcatprio = 0;

    switch (prio) {
    case mbp::LogLevel::Error:
        logcatprio = ANDROID_LOG_ERROR;
        break;
    case mbp::LogLevel::Warning:
        logcatprio = ANDROID_LOG_WARN;
        break;
    case mbp::LogLevel::Info:
        logcatprio = ANDROID_LOG_INFO;
        break;
    case mbp::LogLevel::Debug:
        logcatprio = ANDROID_LOG_DEBUG;
        break;
    case mbp::LogLevel::Verbose:
        logcatprio = ANDROID_LOG_VERBOSE;
        break;
    }

    __android_log_print(logcatprio, LOG_TAG, "%s", msg.c_str());
}

static mbp::LogFunction logger = &logcat;

#else

static void stdlog(mbp::LogLevel prio, const std::string &msg)
{
    const char *fmt = "%s\n";

    switch (prio) {
    case mbp::LogLevel::Debug:
        fmt = "[D] %s\n";
        break;
    case mbp::LogLevel::Error:
        fmt = "[E] %s\n";
        break;
    case mbp::LogLevel::Info:
        fmt = "[I] %s\n";
        break;
    case mbp::LogLevel::Verbose:
        fmt = "[V] %s\n";
        break;
    case mbp::LogLevel::Warning:
        fmt = "[W] %s\n";
        break;
    }

    printf(fmt, msg.c_str());;
}

static mbp::LogFunction logger = stdlog;

#endif


namespace mbp {

void setLogCallback(LogFunction cb)
{
    logger = cb;
}

}

void log(mbp::LogLevel level, const std::string &str)
{
    if (logger) {
        logger(level, str);
    }
}
