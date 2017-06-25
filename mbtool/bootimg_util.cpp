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

#include "bootimg_util.h"

#include <cerrno>
#include <cstdio>
#include <cstring>

#include <unistd.h>

#include "mblog/logging.h"

#define BUF_SIZE    10240

typedef std::unique_ptr<FILE, decltype(fclose) *> ScopedFILE;

namespace mb
{

bool bi_copy_data_to_fd(MbBiReader *bir, int fd)
{
    int ret;
    char buf[BUF_SIZE];
    size_t n_read;
    ssize_t n_written;
    size_t remain;

    while ((ret = mb_bi_reader_read_data(bir, buf, sizeof(buf), &n_read))
            == MB_BI_OK) {
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

    if (ret != MB_BI_EOF) {
        LOGE("Failed to read boot image entry data: %s",
             mb_bi_reader_error_string(bir));
        return false;
    }

    return true;
}

bool bi_copy_file_to_data(const std::string &path, MbBiWriter *biw)
{
    ScopedFILE fp(fopen(path.c_str(), "rb"), fclose);
    if (!fp) {
        LOGE("%s: Failed to open for reading: %s",
             path.c_str(), strerror(errno));
        return false;
    }

    char buf[10240];
    size_t n;

    while (true) {
        n = fread(buf, 1, sizeof(buf), fp.get());

        size_t bytes_written;

        if (mb_bi_writer_write_data(biw, buf, n, &bytes_written) != MB_BI_OK
                || bytes_written != n) {
            LOGE("Failed to write entry data: %s",
                 mb_bi_writer_error_string(biw));
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

bool bi_copy_data_to_file(MbBiReader *bir, const std::string &path)
{
    ScopedFILE fp(fopen(path.c_str(), "wb"), fclose);
    if (!fp) {
        LOGE("%s: Failed to open for writing: %s",
             path.c_str(), strerror(errno));
        return false;
    }

    int ret;
    char buf[10240];
    size_t n;

    while ((ret = mb_bi_reader_read_data(bir, buf, sizeof(buf), &n))
            == MB_BI_OK) {
        if (fwrite(buf, 1, n, fp.get()) != n) {
            LOGE("%s: Failed to write data: %s",
                 path.c_str(), strerror(errno));
            return false;
        }
    }

    if (ret != MB_BI_EOF) {
        LOGE("Failed to read entry data: %s",
             mb_bi_reader_error_string(bir));
        return false;
    }

    if (fclose(fp.release()) < 0) {
        LOGE("%s: Failed to close file: %s",
             path.c_str(), strerror(errno));
        return false;
    }

    return true;
}

bool bi_copy_data_to_data(MbBiReader *bir, MbBiWriter *biw)
{
    int ret;
    char buf[10240];
    size_t n_read;
    size_t n_written;

    while ((ret = mb_bi_reader_read_data(bir, buf, sizeof(buf), &n_read))
            == MB_BI_OK) {
        ret = mb_bi_writer_write_data(biw, buf, n_read, &n_written);
        if (ret != MB_BI_OK || n_read != n_written) {
            LOGE("Failed to write entry data: %s",
                 mb_bi_writer_error_string(biw));
            return false;
        }
    }

    if (ret != MB_BI_EOF) {
        LOGE("Failed to read boot image entry data: %s",
             mb_bi_reader_error_string(bir));
        return false;
    }

    return true;
}

}
