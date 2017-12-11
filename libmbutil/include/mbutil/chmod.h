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

enum class ChmodFlag : uint8_t
{
    Recursive   = 1 << 0,
};
MB_DECLARE_FLAGS(ChmodFlags, ChmodFlag)
MB_DECLARE_OPERATORS_FOR_FLAGS(ChmodFlags)

bool chmod(const std::string &path, mode_t perms, ChmodFlags flags);

}
}
