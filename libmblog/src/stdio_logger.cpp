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

#include "mblog/stdio_logger.h"

#include <ctime>

namespace mb
{
namespace log
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
    case LogLevel::Error:
        stdprio = STDLOG_LEVEL_ERROR;
        break;
    case LogLevel::Warning:
        stdprio = STDLOG_LEVEL_WARNING;
        break;
    case LogLevel::Info:
        stdprio = STDLOG_LEVEL_INFO;
        break;
    case LogLevel::Debug:
        stdprio = STDLOG_LEVEL_DEBUG;
        break;
    case LogLevel::Verbose:
        stdprio = STDLOG_LEVEL_VERBOSE;
        break;
    }

#ifndef _WIN32
    if (_show_timestamps) {
        struct timespec res;
        struct tm tm;
        clock_gettime(CLOCK_REALTIME, &res);
        localtime_r(&res.tv_sec, &tm);

        char buf[100];
        strftime(buf, sizeof(buf), "%Y/%m/%d %H:%M:%S %Z", &tm);
        fprintf(_stream, "[%s]", buf);
    }
#endif

    fprintf(_stream, "%s ", stdprio);
    vfprintf(_stream, fmt, ap);
    fprintf(_stream, "\n");
    fflush(_stream);
}

}
}