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

#include "mbbootimg/entry.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>

#include "mbbootimg/defs.h"
#include "mbbootimg/entry_p.h"
#include "mbbootimg/macros_p.h"


MB_BEGIN_C_DECLS

MbBiEntry * mb_bi_entry_new()
{
    MbBiEntry *entry = static_cast<MbBiEntry *>(calloc(1, sizeof(*entry)));
    return entry;
}

void mb_bi_entry_free(MbBiEntry *entry)
{
    mb_bi_entry_clear(entry);
    free(entry);
}


void mb_bi_entry_clear(MbBiEntry *entry)
{
    if (entry) {
        free(entry->field.name);
        memset(entry, 0, sizeof(*entry));
    }
}

MbBiEntry * mb_bi_entry_clone(MbBiEntry *entry)
{
    MbBiEntry *dup;

    dup = mb_bi_entry_new();
    if (!dup) {
        return nullptr;
    }

    // Copy global options
    dup->fields_set = entry->fields_set;

    // Shallow copy trivial field
    dup->field.type = entry->field.type;
    dup->field.size = entry->field.size;

    // Deep copy strings
    bool deep_copy_error =
            (entry->field.name
                    && !(dup->field.name = strdup(entry->field.name)));

    if (deep_copy_error) {
        mb_bi_entry_free(dup);
        return nullptr;
    }

    return dup;
}

// Fields

int mb_bi_entry_type_is_set(MbBiEntry *entry)
{
    return IS_SET(entry, MB_BI_ENTRY_FIELD_TYPE);
}

int mb_bi_entry_type(MbBiEntry *entry)
{
    return entry->field.type;
}

int mb_bi_entry_set_type(MbBiEntry *entry, int type)
{
    SET_FIELD(entry, MB_BI_ENTRY_FIELD_TYPE, type, type);
    return MB_BI_OK;
}

int mb_bi_entry_unset_type(MbBiEntry *entry)
{
    UNSET_FIELD(entry, MB_BI_ENTRY_FIELD_TYPE, type, 0);
    return MB_BI_OK;
}

const char * mb_bi_entry_name(MbBiEntry *entry)
{
    return entry->field.name;
}

int mb_bi_entry_set_name(MbBiEntry *entry, const char *name)
{
    SET_STRING_FIELD(entry, MB_BI_ENTRY_FIELD_NAME, name, name);
    return MB_BI_OK;
}

int mb_bi_entry_size_is_set(MbBiEntry *entry)
{
    return IS_SET(entry, MB_BI_ENTRY_FIELD_SIZE);
}

uint64_t mb_bi_entry_size(MbBiEntry *entry)
{
    return entry->field.size;
}

int mb_bi_entry_set_size(MbBiEntry *entry, uint64_t size)
{
    SET_FIELD(entry, MB_BI_ENTRY_FIELD_SIZE, size, size);
    return MB_BI_OK;
}

int mb_bi_entry_unset_size(MbBiEntry *entry)
{
    UNSET_FIELD(entry, MB_BI_ENTRY_FIELD_SIZE, size, 0);
    return MB_BI_OK;
}

MB_END_C_DECLS
