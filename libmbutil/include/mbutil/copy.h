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

#include "mbcommon/flags.h"

namespace mb
{
namespace util
{

enum class CopyFlag : uint8_t
{
    CopyAttributes  = 1 << 0,
    CopyXattrs      = 1 << 1,
    ExcludeTopLevel = 1 << 2,
    FollowSymlinks  = 1 << 3,
};
MB_DECLARE_FLAGS(CopyFlags, CopyFlag)
MB_DECLARE_OPERATORS_FOR_FLAGS(CopyFlags)

bool copy_data_fd(int fd_source, int fd_target);
bool copy_xattrs(const std::string &source, const std::string &target);
bool copy_stat(const std::string &source, const std::string &target);
bool copy_contents(const std::string &source, const std::string &target);
bool copy_file(const std::string &source, const std::string &target, CopyFlags flags);
bool copy_dir(const std::string &source, const std::string &target, CopyFlags flags);

}
}
