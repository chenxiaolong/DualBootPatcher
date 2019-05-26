/*
 * Copyright (C) 2014-2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include <array>

#include <cstddef>
#include <cstdio>
#include <cstring>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/uio.h>
#include <unistd.h>

namespace mb::log
{

static constexpr std::size_t KMSG_BUF_SIZE                  = 512;

static constexpr char KMSG_LEVEL_DEBUG[]                    = "<7>";
static constexpr char KMSG_LEVEL_INFO[]                     = "<6>";
static constexpr char KMSG_LEVEL_NOTICE[[maybe_unused]][]   = "<5>";
static constexpr char KMSG_LEVEL_WARNING[]                  = "<4>";
static constexpr char KMSG_LEVEL_ERROR[]                    = "<3>";
static constexpr char KMSG_LEVEL_CRITICAL[[maybe_unused]][] = "<2>";
static constexpr char KMSG_LEVEL_ALERT[[maybe_unused]][]    = "<1>";
static constexpr char KMSG_LEVEL_EMERG[[maybe_unused]][]    = "<0>";
static constexpr char KMSG_LEVEL_DEFAULT[]                  = "<d>";

KmsgLogger::KmsgLogger(bool force_error_prio)
    : _force_error_prio(force_error_prio)
{
    static constexpr int open_mode = O_WRONLY | O_NOCTTY | O_CLOEXEC;
    static constexpr char kmsg[] = "/dev/kmsg";
    static constexpr char kmsg2[] = "/dev/kmsg.mblog";

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

void KmsgLogger::log(const LogRecord &rec)
{
    if (_fd < 0) {
        return;
    }

    const char *kprio = KMSG_LEVEL_DEFAULT;

    if (_force_error_prio) {
        kprio = KMSG_LEVEL_ERROR;
    } else {
        switch (rec.prio) {
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

    std::array<iovec, 3> iov;
    iov[0].iov_base = const_cast<char *>(kprio);
    iov[0].iov_len = strlen(kprio);
    iov[1].iov_base = const_cast<char *>(rec.fmt_msg.c_str());
    iov[1].iov_len = rec.fmt_msg.size();
    iov[2].iov_base = const_cast<char *>("\n");
    iov[2].iov_len = 1;

    if (iov[0].iov_len + iov[1].iov_len >= KMSG_BUF_SIZE) {
        static constexpr char trunc[] = " [trunc...]\n";
        iov[1].iov_len = KMSG_BUF_SIZE - iov[0].iov_len - sizeof(trunc) + 1;
        iov[2].iov_base = const_cast<char *>(trunc);
        iov[2].iov_len = sizeof(trunc) - 1;
    }

    writev(_fd, iov.data(), iov.size());
}

bool KmsgLogger::formatted()
{
    return true;
}

}
