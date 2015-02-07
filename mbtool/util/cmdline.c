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

#include "util/cmdline.h"

#include <string.h>

#include "util/file.h"
#include "util/string.h"

int mb_kernel_cmdline_get_option(const char *option, char **out)
{
    char *line = NULL;
    if (mb_file_first_line("/proc/cmdline", &line) < 0) {
        goto error;
    }

    char *temp;
    char *token;

    token = strtok_r(line, " ", &temp);
    while (token != NULL) {
        if (mb_starts_with(token, option)) {
            char *p = token + strlen(option);
            if (*p == '\0' || *p == ' ') {
                // No value
                *out = strdup("");
                goto success;
            } else if (*p == '=') {
                // Has value
                char *end = strchr(line, ' ');
                if (end) {
                    // Not at end of string
                    size_t len = end - p - 1;
                    *out = malloc(len + 1);
                    memcpy(*out, p + 1, len);
                    (*out)[len] = '\0';
                    goto success;
                } else {
                    // End of string
                    *out = strdup(p + 1);
                    goto success;
                }
            } else {
                // Option did not match entire string
            }
        }

        token = strtok_r(NULL, " ", &temp);
    }

error:
    free(line);
    return -1;

success:
    free(line);
    return 0;
}