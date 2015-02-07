/*
 * Copyright (C) 2014  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "util/path.h"

#include <errno.h>
#include <stdarg.h>
#include <string.h>

char * mb_path_build(char *path1, ...)
{
    size_t length = 0;
    char *current = NULL;
    va_list argp;

    if (!path1) {
        return NULL;
    }

    length += strlen(path1);

    va_start(argp, path1);

    while ((current = va_arg(argp, char *))) {
        // For slash
        length += 1;
        length += strlen(current);
    }

    va_end(argp);

    // NULL-terminator
    length += 1;

    char *newpath = malloc(length);
    if (!newpath) {
        errno = ENOMEM;
        return NULL;
    }

    newpath[0] = '\0';

    strlcat(newpath, path1, length);

    va_start(argp, path1);

    while ((current = va_arg(argp, char *))) {
        strlcat(newpath, "/", length);
        strlcat(newpath, current, length);
    }

    va_end(argp);

    return newpath;
}