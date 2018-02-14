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

#include "mbcommon/outcome.h"

namespace mb::util
{

oc::result<void> create_empty_file(const std::string &path);
oc::result<std::string> file_first_line(const std::string &path);
oc::result<void> file_write_data(const std::string &path,
                                 const void *data, size_t size);
oc::result<bool> file_find_one_of(const std::string &path,
                                  const std::vector<std::string> &items);
oc::result<std::vector<unsigned char>> file_read_all(const std::string &path);

oc::result<uint64_t> get_blockdev_size(const std::string &path);

}
