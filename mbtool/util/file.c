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

#include "util/file.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

int mb_create_empty_file(const char *path)
{
    int fd;
    if ((fd = open(path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR |
                                           S_IRGRP | S_IWGRP |
                                           S_IROTH | S_IWOTH)) < 0) {
        return -1;
    }

    close(fd);
    return 0;
}

int mb_file_first_line(const char *path, char **line_out)
{
    FILE *fp = NULL;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    fp = fopen(path, "rb");
    if (!fp) {
        goto error;
    }

    if ((read = getline(&line, &len, fp)) < 0) {
        goto error;
    }

    if (line[read - 1] == '\n') {
        line[read - 1] = '\0';
        --read;
    }

    fclose(fp);

    *line_out = line;
    return 0;

error:
    if (fp) {
        fclose(fp);
    }

    free(line);
    return -1;
}

int mb_file_write_data(const char *path, const void *data, size_t size)
{
    FILE *fp;
    fp = fopen(path, "wb");
    if (!fp) {
        goto error;
    }

    int saved_errno;

    ssize_t nwritten;

    do {
        if ((nwritten = fwrite(data, 1, size, fp)) < 0) {
            goto error;
        }

        size -= nwritten;
        data += nwritten;
    } while (size > 0);

    if (fclose(fp) < 0) {
        fp = NULL;
        goto error;
    }

    return 0;

error:
    saved_errno = errno;

    if (fp) {
        fclose(fp);
    }

    errno = saved_errno;
    return -1;
}
