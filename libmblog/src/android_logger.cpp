/*
 * Copyright (C) 2014-2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
 *
 * This file is part of DualBootPatcher
 *
 * DualBootPatcher is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * DualBootPatcher is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with DualBootPatcher.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mblog/android_logger.h"

#include <android/log.h>

#include "mblog/logging.h"

namespace mb
{
namespace log
{

void AndroidLogger::log(LogLevel prio, const char *fmt, va_list ap)
{
    int logcatprio;

    switch (prio) {
    case LogLevel::Error:
        logcatprio = ANDROID_LOG_ERROR;
        break;
    case LogLevel::Warning:
        logcatprio = ANDROID_LOG_WARN;
        break;
    case LogLevel::Info:
        logcatprio = ANDROID_LOG_INFO;
        break;
    case LogLevel::Debug:
        logcatprio = ANDROID_LOG_DEBUG;
        break;
    case LogLevel::Verbose:
        logcatprio = ANDROID_LOG_VERBOSE;
        break;
    default:
        return;
    }

    __android_log_vprint(logcatprio, get_log_tag(), fmt, ap);
}

}
}