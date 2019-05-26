/*
 * Copyright (C) 2015-2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include <cstddef>
#include <cstdint>

#include "mbbootimg/header.h"

namespace mb::bootimg::mtk
{

constexpr char MTK_MAGIC[] = "\x88\x16\x88\x58";
constexpr size_t MTK_MAGIC_SIZE = 4;
constexpr size_t MTK_TYPE_SIZE = 32;
constexpr size_t MTK_UNUSED_SIZE = 472;

constexpr HeaderFields SUPPORTED_FIELDS =
        HeaderField::KernelSize
        | HeaderField::KernelAddress
        | HeaderField::RamdiskSize
        | HeaderField::RamdiskAddress
        | HeaderField::SecondbootSize
        | HeaderField::SecondbootAddress
        | HeaderField::KernelTagsAddress
        | HeaderField::PageSize
        | HeaderField::DeviceTreeSize
        | HeaderField::Unused
        | HeaderField::BoardName
        | HeaderField::KernelCmdline
        | HeaderField::Id;

}
