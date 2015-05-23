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

#include "util/loopdev.h"

#include <vector>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>

#include <linux/loop.h>

#include "util/finally.h"
#include "util/string.h"


// See https://lkml.org/lkml/2011/7/30/110


#define LOOP_CONTROL    "/dev/loop-control"
#define LOOP_FMT        "/dev/block/loop%d"


namespace mb
{
namespace util
{

std::string loopdev_find_unused(void)
{
    int fd = -1;
    int n;

    if ((fd = open(LOOP_CONTROL, O_RDWR)) < 0) {
        return std::string();
    }

    auto close_fd = finally([&] {
        close(fd);
    });

    if ((n = ioctl(fd, LOOP_CTL_GET_FREE)) < 0) {
        return std::string();
    }

    return format(LOOP_FMT, n);
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
