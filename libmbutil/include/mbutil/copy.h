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

namespace mb
{
namespace util
{

enum CopyFlags : int
{
    COPY_ATTRIBUTES          = 0x1,
    COPY_XATTRS              = 0x2,
    COPY_EXCLUDE_TOP_LEVEL   = 0x4,
    COPY_FOLLOW_SYMLINKS     = 0x8
};

bool copy_data_fd(int fd_source, int fd_target);
bool copy_xattrs(const std::string &source, const std::string &target);
bool copy_stat(const std::string &source, const std::string &target);
bool copy_contents(const std::string &source, const std::string &target);
bool copy_file(const std::string &source, const std::string &target, int flags);
bool copy_dir(const std::string &source, const std::string &target, int flags);

}
}
