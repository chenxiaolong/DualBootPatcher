/*
 * Copyright (C) 2014  Xiao-Long Chen <chenxiaolong@cxl.epac.to>
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

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <linux/loop.h>


// See https://lkml.org/lkml/2011/7/30/110


#define LOOP_CONTROL "/dev/loop-control"
#define LOOP_PREFIX  "/dev/block/loop"


char * mb_loopdev_find_unused(void)
{
    int saved_errno;
    int fd = -1;
    int n;

    if ((fd = open(LOOP_CONTROL, O_RDWR)) < 0) {
        goto error;
    }

    if ((n = ioctl(fd, LOOP_CTL_GET_FREE)) < 0) {
        goto error;
    }

    close(fd);
    fd = -1;

    size_t len = strlen(LOOP_PREFIX) + 2;
    for (int m = n; m /= 10; ++len);

    char *path = malloc(len);
    if (!path) {
        errno = ENOMEM;
        goto error;
    }

    snprintf(path, len, LOOP_PREFIX "%d", n);

    return path;

error:
    saved_errno = errno;

    if (fd > 0) {
        close(fd);
    }

    errno = saved_errno;

    return NULL;
}

int mb_loopdev_setup_device(const char *loopdev, const char *file,
                            uint64_t offset, int ro)
{
    int saved_errno;
    int ffd = -1;
    int lfd = -1;

    struct loop_info64 loopinfo;

    if ((ffd = open(file, ro ? O_RDONLY : O_RDWR)) < 0) {
        goto error;
    }

    if ((lfd = open(loopdev, ro ? O_RDONLY : O_RDWR)) < 0) {
        goto error;
    }

    memset(&loopinfo, 0, sizeof(struct loop_info64));
    loopinfo.lo_offset = offset;

    if (ioctl(lfd, LOOP_SET_FD, ffd) < 0) {
        goto error;
    }

    if (ioctl(lfd, LOOP_SET_STATUS64, &loopinfo) < 0) {
        ioctl(lfd, LOOP_CLR_FD, 0);
        goto error;
    }

    close(ffd);
    ffd = -1;

    close(lfd);
    lfd = -1;

    return 0;

error:
    saved_errno = errno;

    if (ffd > 0) {
        close(ffd);
    }
    if (lfd > 0) {
        close(lfd);
    }

    errno = saved_errno;

    return -1;
}

int mb_loopdev_remove_device(const char *loopdev)
{
    int lfd;
    if ((lfd = open(loopdev, O_RDONLY)) < 0) {
        return -1;
    }
    int ret = ioctl(lfd, LOOP_CLR_FD, 0);
    close(lfd);
    return ret;
}
