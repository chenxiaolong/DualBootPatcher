/*
 * Copyright (C) 2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbcommon/common.h"

#include <system_error>

#ifdef _WIN32
#  include <windef.h>
#endif

namespace mb
{

[[nodiscard]] MB_EXPORT std::error_code ec_from_errno(int errno_value);
[[nodiscard]] MB_EXPORT std::error_code ec_from_errno();

#ifdef _WIN32
MB_EXPORT const std::error_category & win32_error_category();

[[nodiscard]] MB_EXPORT std::error_code ec_from_win32(DWORD error_value);
[[nodiscard]] MB_EXPORT std::error_code ec_from_win32();
#endif

}
