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

#include <sys/types.h>

namespace mb
{
namespace util
{

enum ChownFlags : int
{
    CHOWN_FOLLOW_SYMLINKS = 0x1,
    CHOWN_RECURSIVE       = 0x2
};

bool chown(const std::string &path,
           const std::string &user,
           const std::string &group,
           int flags);
bool chown(const std::string &path,
           uid_t user,
           gid_t group,
           int flags);

}
}
