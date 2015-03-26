/*
 * Copyright (C) 2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
 *
 * This file is part of MultiBootPatcher
 *
 * MultiBootPatcher is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MultiBootPatcher is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MultiBootPatcher.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <cstdint>

#define BOOT_MAGIC              "ANDROID!"
#define BOOT_MAGIC_SIZE         8
#define BOOT_NAME_SIZE          16
#define BOOT_ARGS_SIZE          512

struct BootImageHeader
{
    unsigned char magic[BOOT_MAGIC_SIZE];

    uint32_t kernel_size;   /* size in bytes */
    uint32_t kernel_addr;   /* physical load addr */

    uint32_t ramdisk_size;  /* size in bytes */
    uint32_t ramdisk_addr;  /* physical load addr */

    uint32_t second_size;   /* size in bytes */
    uint32_t second_addr;   /* physical load addr */

    uint32_t tags_addr;     /* physical addr for kernel tags */
    uint32_t page_size;     /* flash page size we assume */
    uint32_t dt_size;       /* device tree in bytes */
    uint32_t unused;        /* future expansion: should be 0 */
    unsigned char name[BOOT_NAME_SIZE]; /* asciiz product name */

    unsigned char cmdline[BOOT_ARGS_SIZE];

    uint32_t id[8]; /* timestamp / checksum / sha1 / etc */
};