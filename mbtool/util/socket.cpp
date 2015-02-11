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

#include "util/socket.h"

#include <vector>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

namespace MB {

int64_t socket_read(int fd, void *buf, int64_t size)
{
    int32_t bytes_read = 0;
    int n;

    while (bytes_read < size) {
        n = read(fd, static_cast<char *>(buf) + bytes_read, size - bytes_read);
        if (n <= 0) {
            return n;
        }

        bytes_read += n;
    }

    return bytes_read;
}

int64_t socket_write(int fd, const void *buf, int64_t size)
{
    int32_t bytes_written = 0;
    int n;

    while (bytes_written < size) {
        n = write(fd, static_cast<const char *>(buf) + bytes_written,
                  size - bytes_written);
        if (n <= 0) {
            return n;
        }

        bytes_written += n;
    }

    return bytes_written;
}

#define read_int_type(TYPE) \
    TYPE buf; \
    if (socket_read(fd, &buf, sizeof(TYPE)) == sizeof(TYPE)) { \
        *result = buf; \
        return true; \
    } \
    return false;

#define write_int_type(TYPE) \
    if (socket_write(fd, &n, sizeof(TYPE)) == sizeof(TYPE)) { \
        return true; \
    } \
    return false;

bool socket_read_uint16(int fd, uint16_t *result)
{
    read_int_type(uint16_t);
}

bool socket_write_uint16(int fd, uint16_t n)
{
    write_int_type(uint16_t);
}

bool socket_read_uint32(int fd, uint32_t *result)
{
    read_int_type(uint32_t);
}

bool socket_write_uint32(int fd, uint32_t n)
{
    write_int_type(uint32_t);
}

bool socket_read_uint64(int fd, uint64_t *result)
{
    read_int_type(uint64_t);
}

bool socket_write_uint64(int fd, uint64_t n)
{
    write_int_type(uint64_t);
}

bool socket_read_int16(int fd, int16_t *result)
{
    read_int_type(int16_t);
}

bool socket_write_int16(int fd, int16_t n)
{
    write_int_type(int16_t);
}

bool socket_read_int32(int fd, int32_t *result)
{
    read_int_type(int32_t);
}

bool socket_write_int32(int fd, int32_t n)
{
    write_int_type(int32_t);
}

bool socket_read_int64(int fd, int64_t *result)
{
    read_int_type(int64_t);
}

bool socket_write_int64(int fd, int64_t n)
{
    write_int_type(int64_t);
}

bool socket_read_string(int fd, std::string *result)
{
    int32_t len;
    if (!socket_read_int32(fd, &len)) {
        return false;
    }

    if (len < 0) {
        return false;
    }

    std::vector<char> buf(len);

    if (socket_read(fd, buf.data(), len) == (int64_t) len) {
        result->assign(buf.begin(), buf.end());
        return true;
    }
    return false;
}

bool socket_write_string(int fd, const std::string &str)
{
    if (!socket_write_int32(fd, str.size())) {
        return false;
    }

    if (socket_write(fd, str.data(), str.size()) == (int64_t) str.size()) {
        return true;
    }
    return false;
}

}