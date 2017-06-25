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

#pragma once

#include <string>
#include <vector>

#include <inttypes.h>

namespace mb
{
namespace util
{

ssize_t socket_read(int fd, void *buf, size_t size);
ssize_t socket_write(int fd, const void *buf, size_t size);
bool socket_read_bytes(int fd, std::vector<uint8_t> *result);
bool socket_write_bytes(int fd, const uint8_t *data, size_t len);
bool socket_read_uint16(int fd, uint16_t *result);
bool socket_write_uint16(int fd, uint16_t n);
bool socket_read_uint32(int fd, uint32_t *result);
bool socket_write_uint32(int fd, uint32_t n);
bool socket_read_uint64(int fd, uint64_t *result);
bool socket_write_uint64(int fd, uint64_t n);
bool socket_read_int16(int fd, int16_t *result);
bool socket_write_int16(int fd, int16_t n);
bool socket_read_int32(int fd, int32_t *result);
bool socket_write_int32(int fd, int32_t n);
bool socket_read_int64(int fd, int64_t *result);
bool socket_write_int64(int fd, int64_t n);
bool socket_read_string(int fd, std::string *result);
bool socket_write_string(int fd, const std::string &str);
bool socket_read_string_array(int fd, std::vector<std::string> *result);
bool socket_write_string_array(int fd, const std::vector<std::string> &list);
bool socket_receive_fds(int fd, std::vector<int> *fds);
bool socket_send_fds(int fd, const std::vector<int> &fds);

}
}