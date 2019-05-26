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

#pragma once

#include <string>
#include <vector>

#include <cinttypes>

#include "mbcommon/outcome.h"

namespace mb::util
{

oc::result<size_t> socket_read(int fd, void *buf, size_t size);
oc::result<size_t> socket_write(int fd, const void *buf, size_t size);
oc::result<std::vector<unsigned char>> socket_read_bytes(int fd);
oc::result<void> socket_write_bytes(int fd, const void *data, size_t len);
oc::result<uint16_t> socket_read_uint16(int fd);
oc::result<void> socket_write_uint16(int fd, uint16_t n);
oc::result<uint32_t> socket_read_uint32(int fd);
oc::result<void> socket_write_uint32(int fd, uint32_t n);
oc::result<uint64_t> socket_read_uint64(int fd);
oc::result<void> socket_write_uint64(int fd, uint64_t n);
oc::result<int16_t> socket_read_int16(int fd);
oc::result<void> socket_write_int16(int fd, int16_t n);
oc::result<int32_t> socket_read_int32(int fd);
oc::result<void> socket_write_int32(int fd, int32_t n);
oc::result<int64_t> socket_read_int64(int fd);
oc::result<void> socket_write_int64(int fd, int64_t n);
oc::result<std::string> socket_read_string(int fd);
oc::result<void> socket_write_string(int fd, const std::string &str);
oc::result<std::vector<std::string>> socket_read_string_array(int fd);
oc::result<void> socket_write_string_array(int fd, const std::vector<std::string> &list);
oc::result<void> socket_receive_fds(int fd, std::vector<int> &fds);
oc::result<void> socket_send_fds(int fd, const std::vector<int> &fds);

}
