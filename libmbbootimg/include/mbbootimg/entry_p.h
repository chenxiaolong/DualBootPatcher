/*
 * Copyright (C) 2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include <cstdint>

#define MB_BI_ENTRY_FIELD_TYPE      (1U << 0)
#define MB_BI_ENTRY_FIELD_NAME      (1U << 1)
#define MB_BI_ENTRY_FIELD_SIZE      (1U << 2)

struct MbBiEntry
{
    // Bitmap of fields that are set
    uint64_t fields_set;

    struct {
        // Entry type
        int type;
        // Entry name
        char *name;
        // Entry size
        uint64_t size;
    } field;
};
