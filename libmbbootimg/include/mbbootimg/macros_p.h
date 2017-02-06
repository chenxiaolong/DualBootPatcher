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

#include "mbbootimg/guard_p.h"

#define IS_SUPPORTED(STRUCT, FLAG) \
    ((STRUCT)->fields_supported & (FLAG))

#define IS_SET(STRUCT, FLAG) \
    ((STRUCT)->fields_set & (FLAG))

#define ENSURE_SUPPORTED(STRUCT, FLAG) \
    do { \
        if (!IS_SUPPORTED(STRUCT, FLAG)) { \
            return MB_BI_UNSUPPORTED; \
        } \
    } while (0)

#define SET_FIELD(STRUCT, FLAG, FIELD, VALUE) \
    do { \
        (STRUCT)->field.FIELD = (VALUE); \
        (STRUCT)->fields_set |= (FLAG); \
    } while (0)

#define UNSET_FIELD(STRUCT, FLAG, FIELD, VALUE) \
    do { \
        (STRUCT)->field.FIELD = (VALUE); \
        (STRUCT)->fields_set &= ~(FLAG); \
    } while (0)

#define SET_STRING_FIELD(STRUCT, FLAG, FIELD, VALUE) \
    do { \
        free((STRUCT)->field.FIELD); \
        if (VALUE) { \
            char *dup = strdup(VALUE); \
            if (!dup) { \
                return MB_BI_FAILED; \
            } \
            SET_FIELD(STRUCT, FLAG, FIELD, dup); \
        } else { \
            UNSET_FIELD(STRUCT, FLAG, FIELD, nullptr); \
        } \
    } while (0)
