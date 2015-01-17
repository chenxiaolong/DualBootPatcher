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

#include "util/ext4.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "util/logging.h"

int mb_ext4_get_info(const char *path, struct ext4_info *info)
{
    void *map = MAP_FAILED;
    int fd = -1;

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        LOGE("%s: Failed to open: %s", path, strerror(errno));
        goto error;
    }

    // ext4 superblock is 1024 bytes at offset of 1024 bytes
    map = mmap(NULL, 2048, PROT_READ, MAP_PRIVATE, fd, 0);
    if (map == MAP_FAILED) {
        LOGE("%s: Failed to mmap: %s", path, strerror(errno));
        goto error;
    }

    void *superblock = map + 0x400;

    if (memcmp(superblock + 0x38, "\x53\xef", 2) != 0) { // little-endian
        LOGE("%s: Not an ext4 image", path);
        goto error;
    }

    memcpy(&info->magic, superblock + 0x38, sizeof(info->magic));
    memcpy(&info->block_count, superblock + 0x4, sizeof(info->block_count));

    uint32_t s_log_block_size;
    memcpy(&s_log_block_size, superblock + 0x18, sizeof(s_log_block_size));
    info->block_size = 1 << (s_log_block_size + 10);

    munmap(map, 2048);
    close(fd);

    return 0;

error:
    if (map != MAP_FAILED) {
        munmap(map, 2048);
    }
    if (fd >= 0) {
        close(fd);
    }
    return -1;
}
