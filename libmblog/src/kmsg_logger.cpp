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

#include "mblog/kmsg_logger.h"

#include <cstdio>
#include <cstring>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <unistd.h>

#include "mblog/logging.h"

namespace mb
{
namespace log
{

#define KMSG_LEVEL_DEBUG    "<7>"
#define KMSG_LEVEL_INFO     "<6>"
#define KMSG_LEVEL_NOTICE   "<5>"
#define KMSG_LEVEL_WARNING  "<4>"
#define KMSG_LEVEL_ERROR    "<3>"
#define KMSG_LEVEL_CRITICAL "<2>"
#define KMSG_LEVEL_ALERT    "<1>"
#define KMSG_LEVEL_EMERG    "<0>"
#define KMSG_LEVEL_DEFAULT  "<d>"

KmsgLogger::KmsgLogger(bool force_error_prio)
    : _force_error_prio(force_error_prio)
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

    const char *kprio;

    if (_force_error_prio) {
        kprio = KMSG_LEVEL_ERROR;
    } else {
        kprio = KMSG_LEVEL_DEFAULT;

        switch (prio) {
        case LogLevel::Error:
            kprio = KMSG_LEVEL_ERROR;
            break;
        case LogLevel::Warning:
            kprio = KMSG_LEVEL_WARNING;
            break;
        case LogLevel::Info:
            kprio = KMSG_LEVEL_INFO;
            break;
        case LogLevel::Debug:
            kprio = KMSG_LEVEL_DEBUG;
            break;
        case LogLevel::Verbose:
            kprio = KMSG_LEVEL_DEFAULT;
            break;
        }
    }

    char new_fmt[64];
    if (snprintf(new_fmt, sizeof(new_fmt), "%s%s: %s\n",
                 kprio, get_log_tag(), fmt) >= (int) sizeof(new_fmt)) {
        // Doesn't fit
        return;
    }

    std::size_t len = vsnprintf(_buf, KMSG_BUF_SIZE, new_fmt, ap);

    // Make user aware of any truncation
    if (len >= KMSG_BUF_SIZE) {
        static const char trunc[] = " [trunc...]\n";
        memcpy(_buf + sizeof(_buf) - sizeof(trunc), trunc, sizeof(trunc));
    }

    write(_fd, _buf, strlen(_buf));
    //vdprintf(_fd, new_fmt.c_str(), ap);
}

}
}
