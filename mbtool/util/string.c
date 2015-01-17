/*
 * Copyright (C) 2014  Xiao-Long Chen <chenxiaolong@cxl.epac.to>
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

#include "util/string.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int mb_starts_with(const char *string, const char *prefix)
{
    while (*string && *prefix) {
        if (*string++ != *prefix++) {
            return 0;
        }
    }

    if (*prefix && !*string) {
        // Prefix starts with string
        return 0;
    }

    return 1;
}

int mb_ends_with(const char *string, const char *suffix)
{
    size_t string_len = strlen(string);
    size_t suffix_len = strlen(suffix);

    if (string_len < suffix_len) {
        return 0;
    }

    return strcmp(string + string_len - suffix_len, suffix) == 0;
}

char * mb_uint64_to_string(uint64_t number, const char *prefix, const char *suffix) {
    size_t len = 2; // '\0', and first digit

    if (prefix) {
        len += strlen(prefix);
    }
    if (suffix) {
        len += strlen(suffix);
    }

    for (uint64_t n = number; n /= 10; ++len);

    char *str = malloc(len);
    if (str) {
        snprintf(str, len, "%s%" PRIu64 "%s",
                 prefix ? prefix : "", number, suffix ? suffix : "");
    }
    return str;
}
