/*
 * Copyright (C) 2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#define SUPPORTS_KERNEL_ADDRESS              0x1llu
#define SUPPORTS_RAMDISK_ADDRESS             0x2llu
#define SUPPORTS_SECOND_ADDRESS              0x4llu
#define SUPPORTS_TAGS_ADDRESS                0x8llu
#define SUPPORTS_OFFSET_BASE                 \
        (SUPPORTS_KERNEL_ADDRESS             \
        | SUPPORTS_RAMDISK_ADDRESS           \
        | SUPPORTS_SECOND_ADDRESS            \
        | SUPPORTS_TAGS_ADDRESS)
#define SUPPORTS_IPL_ADDRESS                0x10llu
#define SUPPORTS_RPM_ADDRESS                0x20llu
#define SUPPORTS_APPSBL_ADDRESS             0x40llu
#define SUPPORTS_PAGE_SIZE                  0x80llu
#define SUPPORTS_BOARD_NAME                0x100llu
#define SUPPORTS_CMDLINE                   0x200llu
#define SUPPORTS_KERNEL_IMAGE              0x400llu
#define SUPPORTS_RAMDISK_IMAGE             0x800llu
#define SUPPORTS_SECOND_IMAGE             0x1000llu
#define SUPPORTS_DT_IMAGE                 0x2000llu
#define SUPPORTS_ABOOT_IMAGE              0x4000llu
#define SUPPORTS_KERNEL_MTKHDR            0x8000llu
#define SUPPORTS_RAMDISK_MTKHDR          0x10000llu
#define SUPPORTS_IPL_IMAGE               0x20000llu
#define SUPPORTS_RPM_IMAGE               0x40000llu
#define SUPPORTS_APPSBL_IMAGE            0x80000llu
#define SUPPORTS_ENTRYPOINT             0x100000llu
