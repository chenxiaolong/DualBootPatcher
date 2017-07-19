/*
 * Copyright (C) 2014-2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbdevice/internal/array.h"

#include <stdlib.h>
#include <string.h>

size_t string_array_length(char const * const *array)
{
    size_t n = 0;
    for (char const * const *iter = array; *iter; ++iter, ++n);
    return n;
}

char ** string_array_dup(char const * const *array)
{
    size_t n = string_array_length(array);
    size_t size = (n + 1) * sizeof(char *);
    char **new_array = NULL;

    new_array = (char **) malloc(size);
    if (!new_array) {
        goto error;
    }

    memset(new_array, 0, size);

    for (size_t i = 0; i < n; ++i) {
        new_array[i] = strdup(array[i]);
        if (!new_array[i]) {
            goto error;
        }
    }

    return new_array;

error:
    string_array_free(new_array);
    return NULL;
}

char ** string_array_new(size_t n)
{
    size_t size = (n + 1) * sizeof(char *);
    char **array = (char **) malloc(size);
    if (!array) {
        return NULL;
    }
    memset(array, 0, size);
    return array;
}

void string_array_free(char **array)
{
    if (array) {
        for (char **iter = array; *iter; ++iter) {
            free(*iter);
        }
        free(array);
    }
}
