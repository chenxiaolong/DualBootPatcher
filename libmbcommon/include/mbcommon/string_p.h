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

#include "mbcommon/guard_p.h"

#include <cstddef>

#include "mbcommon/common.h"

/*! \cond INTERNAL */
MB_BEGIN_C_DECLS

// Wrap libc functions that aren't available on some platforms

void * _mb_mempcpy(void *dest, const void *src, size_t n);

void * _mb_memmem(const void *haystack, size_t haystacklen,
                  const void *needle, size_t needlelen);

MB_END_C_DECLS
/*! \endcond */
