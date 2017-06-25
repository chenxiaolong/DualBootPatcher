/*
 * Copyright (C) 2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "archive_util.h"

#include <cerrno>
#include <cstring>

#include "mblog/logging.h"

#define BUF_SIZE    10240

namespace mb
{

bool la_copy_data_to_fd(archive *a, int fd)
{
    char buf[BUF_SIZE];
    la_ssize_t n_read;
    ssize_t n_written;
    la_ssize_t remain;

    while ((n_read = archive_read_data(a, buf, sizeof(buf))) > 0) {
        remain = n_read;

        while (remain > 0) {
            n_written = write(fd, buf + (n_read - remain), remain);
            if (n_written <= 0) {
                LOGE("Failed to write data: %s", strerror(errno));
                return false;
            }

            remain -= n_written;
        }
    }

    if (n_read < 0) {
        LOGE("Failed to read archive entry data: %s",
             archive_error_string(a));
        return false;
    }

    return true;
}

}
