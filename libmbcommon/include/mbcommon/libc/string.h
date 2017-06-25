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

#include "mbcommon/common.h"

#ifdef __cplusplus
#  include <cstdarg>
#  include <cstdbool>
#  include <cstddef>
#else
#  include <stdarg.h>
#  include <stdbool.h>
#  include <stddef.h>
#endif

MB_BEGIN_C_DECLS

MB_EXPORT void * mb_memmem(const void *haystack, size_t haystacklen,
                           const void *needle, size_t needlelen);

MB_END_C_DECLS
