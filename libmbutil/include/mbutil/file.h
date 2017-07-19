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

namespace mb
{
namespace util
{

bool create_empty_file(const std::string &path);
bool file_first_line(const std::string &path,
                     std::string *line_out);
bool file_write_data(const std::string &path,
                     const char *data, size_t size);
bool file_find_one_of(const std::string &path, std::vector<std::string> items);
bool file_read_all(const std::string &path,
                   std::vector<unsigned char> *data_out);
bool file_read_all(const std::string &path,
                   unsigned char **data_out,
                   std::size_t *size_out);

bool get_blockdev_size(const char *path, uint64_t *size_out);

}
}
