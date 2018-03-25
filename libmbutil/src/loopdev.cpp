/*
 * Copyright (C) 2014-2018  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbutil/loopdev.h"

#include <vector>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <unistd.h>

#include <linux/loop.h>

#include "mbcommon/error_code.h"
#include "mbcommon/finally.h"
#include "mbcommon/string.h"
#include "mbutil/string.h"


// See https://lkml.org/lkml/2011/7/30/110


#define LOOP_CONTROL    "/dev/loop-control"
#define LOOP_FMT        "/dev/block/loop%d"

#define MAX_LOOPDEVS    1024


namespace mb::util
{

/*!
 * \brief Find empty loopdev by using the new ioctl for /dev/block/loop-control
 *
 * \return Loopdev number or the error code if loop-control does not exist or
 *         the ioctl failed
 */
static oc::result<int> find_loopdev_by_loop_control()
{
    int fd = -1;

    if ((fd = open(LOOP_CONTROL, O_RDWR)) < 0) {
        return ec_from_errno();
    }

    auto close_fd = finally([&] {
        close(fd);
    });

    int n = ioctl(fd, LOOP_CTL_GET_FREE);
    if (n < 0) {
        return ec_from_errno();
    }

    char loopdev[64];
    sprintf(loopdev, LOOP_FMT, n);

    if (mknod(loopdev, S_IFBLK | 0644, static_cast<dev_t>(makedev(7, n))) < 0
            && errno != EEXIST) {
        ec_from_errno();
    }

    return n;
}

/*!
 * \brief Find empty loopdev by dumb scan through /dev/block/loop*
 *
 * \return Loopdev number or the error code if:
 *         - /dev/block/loop# failed to stat where errno != ENOENT
 *         - /dev/block/loop# is not a loop device
 *         - /dev/block/loop# could not be opened
 *         - LOOP_GET_STATUS64 ioctl failed where errno != ENXIO
 */
static oc::result<int> find_loopdev_by_scanning()
{
    int fd;
    char loopdev[64];

    // Avoid /dev/block/loop0 since some installers (ahem, SuperSU) are
    // hardcoded to use it
    for (int n = 1; n < MAX_LOOPDEVS; ++n) {
        loop_info64 loopinfo;
        struct stat sb;

        sprintf(loopdev, LOOP_FMT, n);

        if (mknod(loopdev, S_IFBLK | 0644,
                  static_cast<dev_t>(makedev(7, n))) < 0) {
            if (errno != EEXIST) {
                continue;
            }
        }

        if ((fd = open(loopdev, O_RDWR | O_CLOEXEC)) < 0) {
            // Loopdev does not exist. Great! loopdev_find_unused() will
            // create it
            if (errno == ENOENT) {
                return n;
            } else {
                continue;
            }
        }

        auto close_fd = finally([&] {
            close(fd);
        });

        if (fstat(fd, &sb) < 0) {
            // Uhhhhh...
            continue;
        }

        if (!S_ISBLK(sb.st_mode) || major(sb.st_rdev) != 7) {
            // Device isn't a loop device
            continue;
        }

        if (ioctl(fd, LOOP_GET_STATUS64, &loopinfo) < 0) {
            if (errno == ENXIO) {
                return n;
            } else {
                continue;
            }
        }
    }

    return std::errc::no_such_file_or_directory;
}

oc::result<std::string> loopdev_find_unused()
{
    auto n = find_loopdev_by_loop_control();

    // Also search by scanning if n == 0, since some installers hardcode
    // /dev/block/loop0
    if (!n || n.value() == 0) {
        n = find_loopdev_by_scanning();
    }
    if (!n) {
        return n.as_failure();
    }

    return format(LOOP_FMT, n.value());
}

oc::result<void> loopdev_set_up_device(const std::string &loopdev,
                                       const std::string &file,
                                       uint64_t offset, bool ro)
{
    int ffd = open(file.c_str(), ro ? O_RDONLY : O_RDWR);
    if (ffd < 0) {
        return ec_from_errno();
    }

    auto close_ffd = finally([&] {
        close(ffd);
    });

    int lfd = open(loopdev.c_str(), ro ? O_RDONLY : O_RDWR);
    if (lfd < 0) {
        return ec_from_errno();
    }

    auto close_lfd = finally([&] {
        close(lfd);
    });

    loop_info64 loopinfo = {};

    strlcpy(reinterpret_cast<char *>(loopinfo.lo_file_name), file.c_str(),
            LO_NAME_SIZE);
    loopinfo.lo_offset = offset;

    if (ioctl(lfd, LOOP_SET_FD, ffd) < 0) {
        return ec_from_errno();
    }

    if (ioctl(lfd, LOOP_SET_STATUS64, &loopinfo) < 0) {
        int saved_errno = errno;
        ioctl(lfd, LOOP_CLR_FD, 0);
        return ec_from_errno(saved_errno);
    }

    return oc::success();
}

oc::result<void> loopdev_remove_device(const std::string &loopdev)
{
    int lfd = open(loopdev.c_str(), O_RDONLY);
    if (lfd < 0) {
        return ec_from_errno();
    }

    auto close_fd = finally([&] {
        close(lfd);
    });

    if (ioctl(lfd, LOOP_CLR_FD, 0) < 0) {
        return ec_from_errno();
    }

    return oc::success();
}

}
