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

#include "mbbootimg/header.h"

#define ANDROID_BOOT_MAGIC              "ANDROID!"
#define ANDROID_BOOT_MAGIC_SIZE         8
#define ANDROID_BOOT_NAME_SIZE          16
#define ANDROID_BOOT_ARGS_SIZE          512

#define ANDROID_MAX_HEADER_OFFSET       512

#define SAMSUNG_SEANDROID_MAGIC         "SEANDROIDENFORCE"
#define SAMSUNG_SEANDROID_MAGIC_SIZE    16

#define ANDROID_DEFAULT_PAGE_SIZE       2048u
#define ANDROID_DEFAULT_OFFSET_BASE     0x10000000u
#define ANDROID_DEFAULT_KERNEL_OFFSET   0x00008000u
#define ANDROID_DEFAULT_RAMDISK_OFFSET  0x01000000u
#define ANDROID_DEFAULT_SECOND_OFFSET   0x00f00000u
#define ANDROID_DEFAULT_TAGS_OFFSET     0x00000100u

#define ANDROID_SUPPORTED_FIELDS \
        (MB_BI_HEADER_FIELD_KERNEL_SIZE \
        | MB_BI_HEADER_FIELD_KERNEL_ADDRESS \
        | MB_BI_HEADER_FIELD_RAMDISK_SIZE \
        | MB_BI_HEADER_FIELD_RAMDISK_ADDRESS \
        | MB_BI_HEADER_FIELD_SECONDBOOT_SIZE \
        | MB_BI_HEADER_FIELD_SECONDBOOT_ADDRESS \
        | MB_BI_HEADER_FIELD_KERNEL_TAGS_ADDRESS \
        | MB_BI_HEADER_FIELD_PAGE_SIZE \
        | MB_BI_HEADER_FIELD_DEVICE_TREE_SIZE \
        | MB_BI_HEADER_FIELD_UNUSED \
        | MB_BI_HEADER_FIELD_BOARD_NAME \
        | MB_BI_HEADER_FIELD_KERNEL_CMDLINE \
        | MB_BI_HEADER_FIELD_ID)
