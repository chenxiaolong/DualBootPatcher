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

#define MTK_MAGIC                       "\x88\x16\x88\x58"
#define MTK_MAGIC_SIZE                  4
#define MTK_TYPE_SIZE                   32
#define MTK_UNUSED_SIZE                 472

#define MTK_SUPPORTED_FIELDS \
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
