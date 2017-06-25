/*
 * Copyright (C) 2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbutil/vibrate.h"

#include <cerrno>
#include <cstdio>
#include <cstring>

#include <fcntl.h>
#include <unistd.h>

#include "mblog/logging.h"
#include "mbutil/finally.h"

#define VIBRATOR_PATH           "/sys/class/timed_output/vibrator/enable"

namespace mb
{
namespace util
{

bool vibrate(unsigned int timeout_ms, unsigned int additional_wait_ms)
{
    int fd = open(VIBRATOR_PATH, O_WRONLY);
    if (fd < 0) {
        LOGW("%s: Failed to open: %s", VIBRATOR_PATH, strerror(errno));
        return false;
    }
    auto close_fd = util::finally([&]{
        close(fd);
    });

    if (timeout_ms > 2000) {
        timeout_ms = 2000;
    }

    char buf[20];
    int size = snprintf(buf, sizeof(buf), "%u", timeout_ms);
    if (write(fd, buf, size) < 0) {
        LOGW("%s: Failed to write: %s", VIBRATOR_PATH, strerror(errno));
        return false;
    }

    usleep(1000 * (timeout_ms + additional_wait_ms));

    LOGD("Vibrated for %u ms and waited an additional %u ms",
         timeout_ms, additional_wait_ms);
    return true;
}

}
}
