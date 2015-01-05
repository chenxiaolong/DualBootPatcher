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

#include "util/directory.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

int mb_mkdir_recursive(const char *dir, mode_t mode)
{
    struct stat st;
    char *p;
    char *save_ptr;
    char *temp = NULL;
    char *copy = NULL;
    int len;

    if (!dir || strlen(dir) == 0) {
        errno = EINVAL;
        goto error;
    }

    copy = strdup(dir);
    len = strlen(dir);
    temp = malloc(len + 2);
    temp[0] = '\0';

    if (dir[0] == '/') {
        strcat(temp, "/");
    }

    p = strtok_r(copy, "/", &save_ptr);
    while (p != NULL) {
        strcat(temp, p);
        strcat(temp, "/");

        if (stat(temp, &st) < 0 && mkdir(temp, mode) < 0) {
            goto error;
        }

        p = strtok_r(NULL, "/", &save_ptr);
    }

    free(copy);
    free(temp);
    return 0;

error:
    free(copy);
    free(temp);
    return -1;
}
