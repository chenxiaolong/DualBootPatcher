/*
 * Copyright (C) 2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

// Formats

#define MB_BI_FORMAT_BASE_MASK          0xff0000
#define MB_BI_FORMAT_ANDROID            0x010000
#define MB_BI_FORMAT_BUMP               0x020000
#define MB_BI_FORMAT_LOKI               0x030000
#define MB_BI_FORMAT_MTK                0x040000
#define MB_BI_FORMAT_SONY_ELF           0x050000

#define MB_BI_FORMAT_NAME_ANDROID       "android"
#define MB_BI_FORMAT_NAME_BUMP          "bump"
#define MB_BI_FORMAT_NAME_LOKI          "loki"
#define MB_BI_FORMAT_NAME_MTK           "mtk"
#define MB_BI_FORMAT_NAME_SONY_ELF      "sony_elf"

// Return values

#define MB_BI_EOF                       1
#define MB_BI_OK                        0
#define MB_BI_RETRY                     (-1) // TODO TODO TODO TODO TODO TODO
#define MB_BI_UNSUPPORTED               (-2)
#define MB_BI_WARN                      (-3)
#define MB_BI_FAILED                    (-4)
#define MB_BI_FATAL                     (-5)

// Error types

#define MB_BI_ERROR_NONE                0
#define MB_BI_ERROR_INVALID_ARGUMENT    1
#define MB_BI_ERROR_UNSUPPORTED         2
#define MB_BI_ERROR_FILE_FORMAT         2
#define MB_BI_ERROR_PROGRAMMER_ERROR    3
#define MB_BI_ERROR_INTERNAL_ERROR      4
