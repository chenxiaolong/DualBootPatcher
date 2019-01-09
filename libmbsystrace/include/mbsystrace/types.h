/*
 * Copyright (C) 2018  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include <array>

#include <linux/types.h>

namespace mb::systrace
{

//! Kernel signed long type
using KernelSLong = __kernel_long_t;
//! Kernel unsigned long type
using KernelULong = __kernel_ulong_t;

//! System call number type (unsigned)
using SysCallNum = KernelULong;
//! System call argument type (unsigned)
using SysCallArg = KernelULong;
//! System call arguments array type
using SysCallArgs = std::array<SysCallArg, 6>;
//! System call return value type (signed)
using SysCallRet = KernelSLong;

}
