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

namespace mb::bootimg::android
{

constexpr unsigned char BOOT_MAGIC[]                 = "ANDROID!";
constexpr size_t        BOOT_MAGIC_SIZE              = 8;
constexpr size_t        BOOT_NAME_SIZE               = 16;
constexpr size_t        BOOT_ARGS_SIZE               = 512;

constexpr size_t        MAX_HEADER_OFFSET            = 512;

constexpr unsigned char SAMSUNG_SEANDROID_MAGIC[]    = "SEANDROIDENFORCE";
constexpr size_t        SAMSUNG_SEANDROID_MAGIC_SIZE = 16;

constexpr uint32_t      DEFAULT_PAGE_SIZE            = 2048u;
constexpr uint32_t      DEFAULT_OFFSET_BASE          = 0x10000000u;
constexpr uint32_t      DEFAULT_KERNEL_OFFSET        = 0x00008000u;
constexpr uint32_t      DEFAULT_RAMDISK_OFFSET       = 0x01000000u;
constexpr uint32_t      DEFAULT_SECOND_OFFSET        = 0x00f00000u;
constexpr uint32_t      DEFAULT_TAGS_OFFSET          = 0x00000100u;

constexpr HeaderFields  SUPPORTED_FIELDS             =
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
