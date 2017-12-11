/*
 * Copyright (C) 2014-2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbcommon/flags.h"

namespace mb
{
namespace util
{

enum class ChownFlag : uint8_t
{
    FollowSymlinks  = 1 << 0,
    Recursive       = 1 << 1,
};
MB_DECLARE_FLAGS(ChownFlags, ChownFlag)
MB_DECLARE_OPERATORS_FOR_FLAGS(ChownFlags)

bool chown(const std::string &path,
           const std::string &user,
           const std::string &group,
           ChownFlags flags);
bool chown(const std::string &path,
           uid_t user,
           gid_t group,
           ChownFlags flags);

}
}
