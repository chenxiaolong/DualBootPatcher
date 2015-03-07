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

#pragma once

#include <string>
#include <vector>

#include <archive.h>
#include <archive_entry.h>

namespace mb
{
namespace util
{

struct extract_info {
    std::string from;
    std::string to;
};

struct exists_info {
    std::string path;
    bool exists;
};

int archive_copy_data(archive *in, archive *out);
int archive_copy_header_and_data(archive *in, archive *out,
                                 archive_entry *entry);
bool extract_archive(const std::string &filename, const std::string &target);
bool extract_files(const std::string &filename, const std::string &target,
                   const std::vector<std::string> &files);
bool extract_files2(const std::string &filename,
                    const std::vector<extract_info> &files);
bool archive_exists(const std::string &filename,
                    std::vector<exists_info> &files);

}
}