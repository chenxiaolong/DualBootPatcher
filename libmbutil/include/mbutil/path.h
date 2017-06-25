/*
 * Copyright (C) 2014-2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

std::string get_cwd();
std::string dir_name(const std::string &path);
std::string base_name(const std::string &path);
std::string real_path(const std::string &path);
bool read_link(const std::string &path, std::string *out);
bool inodes_equal(const std::string &path1, const std::string &path2);
std::vector<std::string> path_split(const std::string &path);
std::string path_join(const std::vector<std::string> &components);
void normalize_path(std::vector<std::string> *components);
bool relative_path(const std::string &path, const std::string &start,
                   std::string *out);
int path_compare(const std::string &path1, const std::string &path2);
bool wait_for_path(const char *path, unsigned int timeout_ms);
bool path_exists(const char *path, bool follow_symlinks);

}
}
