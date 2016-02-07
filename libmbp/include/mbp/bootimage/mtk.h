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

#include <vector>
#include <cstdint>


#define MTK_MAGIC       "\x88\x16\x88\x58"
#define MTK_MAGIC_SIZE  4
#define MTK_TYPE_SIZE   32
#define MTK_UNUSED_SIZE 472

struct MtkHeader {
    unsigned char magic[MTK_MAGIC_SIZE];        // MTK magic
    uint32_t size;                              // Image size
    char type[MTK_TYPE_SIZE];                   // Image type
    char unused[MTK_UNUSED_SIZE];               // Unused (all 0xff)
};
