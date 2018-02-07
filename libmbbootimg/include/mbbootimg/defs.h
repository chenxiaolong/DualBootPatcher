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

namespace mb::bootimg
{

// Formats

constexpr int FORMAT_BASE_MASK          = 0xff0000;
constexpr int FORMAT_ANDROID            = 0x010000;
constexpr int FORMAT_BUMP               = 0x020000;
constexpr int FORMAT_LOKI               = 0x030000;
constexpr int FORMAT_MTK                = 0x040000;
constexpr int FORMAT_SONY_ELF           = 0x050000;

constexpr char FORMAT_NAME_ANDROID[]    = "android";
constexpr char FORMAT_NAME_BUMP[]       = "bump";
constexpr char FORMAT_NAME_LOKI[]       = "loki";
constexpr char FORMAT_NAME_MTK[]        = "mtk";
constexpr char FORMAT_NAME_SONY_ELF[]   = "sony_elf";

}
