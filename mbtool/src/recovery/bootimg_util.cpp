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

#include "recovery/bootimg_util.h"

#include <cerrno>
#include <cstdio>
#include <cstring>

#include <unistd.h>

#include "mblog/logging.h"

#define LOG_TAG "mbtool/recovery/bootimg_util"

#define BUF_SIZE    10240

typedef std::unique_ptr<FILE, decltype(fclose) *> ScopedFILE;

using namespace mb::bootimg;

namespace mb
{

bool bi_copy_data_to_fd(Reader &reader, int fd)
{
    char buf[BUF_SIZE];

    while (true) {
        auto n_read = reader.read_data(buf, sizeof(buf));
        if (!n_read) {
            LOGE("Failed to read boot image entry data: %s",
                 n_read.error().message().c_str());
            return false;
        } else if (n_read.value() == 0) {
            break;
        }

        size_t remain = n_read.value();

        while (remain > 0) {
            ssize_t n_written = write(
                    fd, buf + (n_read.value() - remain), remain);
            if (n_written <= 0) {
                LOGE("Failed to write data: %s", strerror(errno));
                return false;
            }

            remain -= static_cast<size_t>(n_written);
        }
    }

    return true;
}

bool bi_copy_file_to_data(const std::string &path, Writer &writer)
{
    ScopedFILE fp(fopen(path.c_str(), "rbe"), fclose);
    if (!fp) {
        LOGE("%s: Failed to open for reading: %s",
             path.c_str(), strerror(errno));
        return false;
    }

    char buf[10240];
    size_t n;

    while (true) {
        n = fread(buf, 1, sizeof(buf), fp.get());

        auto bytes_written = writer.write_data(buf, n);
        if (!bytes_written) {
            LOGE("Failed to write entry data: %s",
                 bytes_written.error().message().c_str());
            return false;
        }

        if (n < sizeof(buf)) {
            if (ferror(fp.get())) {
                LOGE("%s: Failed to read file: %s",
                     path.c_str(), strerror(errno));
            } else {
                break;
            }
        }
    }

    return true;
}

bool bi_copy_data_to_file(Reader &reader, const std::string &path)
{
    ScopedFILE fp(fopen(path.c_str(), "wbe"), fclose);
    if (!fp) {
        LOGE("%s: Failed to open for writing: %s",
             path.c_str(), strerror(errno));
        return false;
    }

    char buf[10240];

    while (true) {
        auto n = reader.read_data(buf, sizeof(buf));
        if (!n) {
            LOGE("Failed to read entry data: %s",
                 n.error().message().c_str());
            return false;
        } else if (n.value() == 0) {
            break;
        }

        if (fwrite(buf, 1, n.value(), fp.get()) != n.value()) {
            LOGE("%s: Failed to write data: %s",
                 path.c_str(), strerror(errno));
            return false;
        }
    }

    if (fclose(fp.release()) < 0) {
        LOGE("%s: Failed to close file: %s",
             path.c_str(), strerror(errno));
        return false;
    }

    return true;
}

bool bi_copy_data_to_data(Reader &reader, Writer &writer)
{
    char buf[10240];

    while (true) {
        auto n_read = reader.read_data(buf, sizeof(buf));
        if (!n_read) {
            LOGE("Failed to read boot image entry data: %s",
                 n_read.error().message().c_str());
            return false;
        } else if (n_read.value() == 0) {
            break;
        }

        auto n_written = writer.write_data(buf, n_read.value());
        if (!n_written) {
            LOGE("Failed to write entry data: %s",
                 n_written.error().message().c_str());
            return false;
        }
    }

    return true;
}

}
