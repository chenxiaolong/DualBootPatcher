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

#include "logging.h"

#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#define KMSG_BUF_SIZE 512

static int kmsg_fd = -1;

void kmsg_init()
{
    if (kmsg_fd < 0) {
        kmsg_fd = open("/dev/kmsg", O_WRONLY);
    }
}

void kmsg_cleanup()
{
    if (kmsg_fd >= 0) {
        close(kmsg_fd);
    }
}

void kmsg_write(const char *fmt, ...)
{
    if (kmsg_fd < 0) {
        kmsg_init();
    }
    if (kmsg_fd < 0) {
        // error...
        return;
    }

    va_list ap;
    va_start(ap, fmt);

    char buf[KMSG_BUF_SIZE];
    vsnprintf(buf, KMSG_BUF_SIZE, fmt, ap);
    buf[KMSG_BUF_SIZE - 1] = '\0';
    write(kmsg_fd, buf, strlen(buf));

    va_end(ap);
}
