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

#include "mbcommon/libc/stdio.h"

#include <cstring>

#if defined(__WIN32) || (defined(__ANDROID_API__) && __ANDROID_API__ < 21)
#  include "mbcommon/external/gnulib/getdelim.h"
#  include "mbcommon/external/gnulib/getline.h"
#endif

#if defined(__WIN32) || (defined(__ANDROID_API__) && __ANDROID_API__ < 21)
#  define getdelim gnulib_getdelim
#  define getline gnulib_getline
#endif

MB_BEGIN_C_DECLS

ssize_t mb_getdelim(char **__restrict lineptr, size_t *__restrict n,
                    int delim, FILE *__restrict stream)
{
    return getdelim(lineptr, n, delim, stream);
}

ssize_t mb_getline(char **__restrict lineptr, size_t *__restrict n,
                   FILE *__restrict stream)
{
    return getline(lineptr, n, stream);
}

MB_END_C_DECLS
