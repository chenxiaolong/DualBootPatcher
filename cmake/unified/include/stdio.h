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

// NDK ships API 21 libc.a
#if __ANDROID_API__ > 21
#  define UNIFIED_STDIO_HACK
#endif

#ifdef UNIFIED_STDIO_HACK
#  pragma push_macro("__ANDROID_API__")
#  undef __ANDROID_API__
#  define __ANDROID_API__ 21
#endif

#include_next <stdio.h>

#ifdef UNIFIED_STDIO_HACK
#  pragma pop_macro("__ANDROID_API__")
#  undef UNIFIED_STDIO_HACK
#endif
