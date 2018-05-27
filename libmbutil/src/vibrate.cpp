/*
 * Copyright (C) 2015-2018  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include <thread>

#include <cstdio>
#include <cstring>

#include <fcntl.h>
#include <unistd.h>

#include "mbcommon/error_code.h"
#include "mbcommon/finally.h"


using namespace std::chrono;
using namespace std::chrono_literals;

namespace mb::util
{

static constexpr char VIBRATOR_PATH[] =
    "/sys/class/timed_output/vibrator/enable";

/**
 * \brief Vibrate and optionally wait
 *
 * \param timeout Vibration duration
 * \param wait Duration to sleep (unrelated to \p timeout)
 *
 * \return Nothing if successful. Otherwise, the error.
 */
oc::result<void> vibrate(milliseconds timeout, milliseconds wait)
{
    int fd = open(VIBRATOR_PATH, O_WRONLY | O_CLOEXEC);
    if (fd < 0) {
        return ec_from_errno();
    }
    auto close_fd = finally([&]{
        close(fd);
    });

    if (timeout > 2s) {
        timeout = 2s;
    }

    if (dprintf(fd, "%u", static_cast<unsigned int>(timeout.count())) < 0) {
        return ec_from_errno();
    }

    std::this_thread::sleep_for(wait);

    return oc::success();
}

}
