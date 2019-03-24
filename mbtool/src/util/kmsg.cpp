/*
 * Copyright (C) 2019  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "util/kmsg.h"

#include <cerrno>
#include <cstring>

#include <fcntl.h>
#include <sys/klog.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>

#include "mbcommon/error_code.h"
#include "mbcommon/outcome.h"

#include "mblog/kmsg_logger.h"
#include "mblog/logging.h"

#include "mbutil/unique_fd.h"

#define LOG_TAG "mbtool/util/kmsg"

namespace mb
{

static oc::result<void> redirect_stdio_null()
{
    static constexpr char name[] = "/dev/__null__";

    if (mknod(name, S_IFCHR | 0600, makedev(1, 3)) < 0) {
        return ec_from_errno();
    }

    // O_CLOEXEC should not be used
    util::UniqueFd fd(open(name, O_RDWR));
    if (fd < 0) {
        return ec_from_errno();
    }

    unlink(name);

    if (dup2(fd, STDIN_FILENO) < 0
            || dup2(fd, STDOUT_FILENO) < 0
            || dup2(fd, STDERR_FILENO) < 0) {
        return ec_from_errno();
    }

    if (fd <= 2) {
        (void) fd.release();
    }

    return oc::success();
}

bool set_kmsg_logging()
{
    if (auto r = redirect_stdio_null(); !r) {
        LOGE("Failed to redirect stdio streams to null: %s",
             r.error().message().c_str());
        return false;
    }

    log::set_logger(std::make_shared<log::KmsgLogger>(true));

    if (klogctl(KLOG_CONSOLE_LEVEL, nullptr, 7) < 0) {
        LOGE("Failed to set kmsg loglevel: %s", strerror(errno));
        return false;
    }

    return true;
}

}
