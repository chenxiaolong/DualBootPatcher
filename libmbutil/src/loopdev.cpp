/*
 * Copyright (C) 2014  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbcommon/string.h"
#include "mbutil/finally.h"
#include "mbutil/string.h"


// See https://lkml.org/lkml/2011/7/30/110


#define LOOP_CONTROL    "/dev/loop-control"
#define LOOP_FMT        "/dev/block/loop%d"

#define MAX_LOOPDEVS    1024


namespace mb
{
namespace util
{

/*!
 * \brief Find empty loopdev by using the new ioctl for /dev/block/loop-control
 *
 * \return Loopdev number or -1 if loop-control does not exist or the ioctl
 *         failed
 */
static int find_loopdev_by_loop_control(void)
{
    int fd = -1;

    if ((fd = open(LOOP_CONTROL, O_RDWR)) < 0) {
        return -1;
    }

    auto close_fd = finally([&] {
        close(fd);
    });

    int n = ioctl(fd, LOOP_CTL_GET_FREE);
    if (n < 0) {
        return -1;
    }

    char loopdev[64];
    sprintf(loopdev, LOOP_FMT, n);

    if (mknod(loopdev, S_IFBLK | 0644, makedev(7, n)) < 0 && errno != EEXIST) {
        return -1;
    }

    return n;
}

/*!
 * \brief Find empty loopdev by dumb scan through /dev/block/loop*
 *
 * \return Loopdev number or -1 if:
 *         - /dev/block/loop# failed to stat where errno != ENOENT
 *         - /dev/block/loop# is not a loop device
 *         - /dev/block/loop# could not be opened
 *         - LOOP_GET_STATUS64 ioctl failed where errno != ENXIO
 */
static int find_loopdev_by_scanning(void)
{
    int fd;
    char loopdev[64];

    // Avoid /dev/block/loop0 since some installers (ahem, SuperSU) are
    // hardcoded to use it
    for (int n = 1; n < MAX_LOOPDEVS; ++n) {
        struct loop_info64 loopinfo;
        struct stat sb;

        sprintf(loopdev, LOOP_FMT, n);

        if (mknod(loopdev, S_IFBLK | 0644, makedev(7, n)) < 0) {
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

    return -1;
}

std::string loopdev_find_unused(void)
{
    int n = find_loopdev_by_loop_control();
    // Also search by scanning if n == 0, since some installers hardcode
    // /dev/block/loop0
    if (n <= 0) {
        n = find_loopdev_by_scanning();
    }
    if (n < 0) {
        return {};
    }

    std::string result;

    char *path = mb_format(LOOP_FMT, n);
    if (path) {
        result = path;
        free(path);
    }

    return result;
}

bool loopdev_set_up_device(const std::string &loopdev, const std::string &file,
                           uint64_t offset, bool ro)
{
    int ffd = -1;
    int lfd = -1;

    struct loop_info64 loopinfo;

    if ((ffd = open(file.c_str(), ro ? O_RDONLY : O_RDWR)) < 0) {
        return false;
    }

    auto close_ffd = finally([&] {
        close(ffd);
    });

    if ((lfd = open(loopdev.c_str(), ro ? O_RDONLY : O_RDWR)) < 0) {
        return false;
    }

    auto close_lfd = finally([&] {
        close(lfd);
    });

    memset(&loopinfo, 0, sizeof(struct loop_info64));
    strlcpy((char *) loopinfo.lo_file_name, file.c_str(), LO_NAME_SIZE);
    loopinfo.lo_offset = offset;

    if (ioctl(lfd, LOOP_SET_FD, ffd) < 0) {
        return false;
    }

    if (ioctl(lfd, LOOP_SET_STATUS64, &loopinfo) < 0) {
        ioctl(lfd, LOOP_CLR_FD, 0);
        return false;
    }

    return true;
}

bool loopdev_remove_device(const std::string &loopdev)
{
    int lfd;
    if ((lfd = open(loopdev.c_str(), O_RDONLY)) < 0) {
        return false;
    }
    int ret = ioctl(lfd, LOOP_CLR_FD, 0);
    close(lfd);
    return ret == 0;
}

}
}
