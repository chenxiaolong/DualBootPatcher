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

#include "mbcommon/libc/string.h"

#include <cstring>

#ifndef __GLIBC__
#  include "mbcommon/external/musl/memmem.h"
#endif

#ifndef __GLIBC__
#  define memmem musl_memmem
#endif

MB_BEGIN_C_DECLS

void * mb_memmem(const void *haystack, size_t haystacklen,
                 const void *needle, size_t needlelen)
{
    return memmem(haystack, haystacklen, needle, needlelen);
}

MB_END_C_DECLS
