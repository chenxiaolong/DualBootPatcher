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

#include "mbcommon/common.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>

/*!
 * \file mbcommon/common.h
 * \brief Common macros for all `libmb*` libraries.
 */

/*!
 * \namespace mb
 *
 * \brief Namespace for all `libmb*` libraries.
 */

void mb_unreachable(const char *file, unsigned long line,
                    const char *func, const char *fmt, ...)
{
    if (file) {
        fprintf(stderr, "(%s:%lu): ", file, line);
    }
    if (func) {
        fprintf(stderr, "%s: ", func);
    }

    if (fmt) {
        va_list ap;
        va_start(ap, fmt);
        fprintf(stderr, "UNREACHABLE: ");
        vfprintf(stderr, fmt, ap);
        va_end(ap);
    } else {
        fprintf(stderr, "ENCOUNTERED UNREACHABLE CODE");
    }
    fprintf(stderr, "\n");

    abort();
}
