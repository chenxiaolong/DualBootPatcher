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

#include "libmiscstuff.h"

#include <sys/stat.h>
#include <sys/vfs.h>

int64_t get_mnt_total_size(const char *mountpoint) {
    struct statfs sfs;

    if (statfs(mountpoint, &sfs) < 0) {
        return 0;
    }

    return sfs.f_bsize * sfs.f_blocks;
}

int64_t get_mnt_avail_size(const char *mountpoint) {
    struct statfs sfs;

    if (statfs(mountpoint, &sfs) < 0) {
        return 0;
    }

    return sfs.f_bsize * sfs.f_bavail;
}

int is_same_file(const char *path1, const char *path2)
{
    struct stat sb1;
    struct stat sb2;

    if (stat(path1, &sb1) < 0 || stat(path2, &sb2) < 0) {
        return 0;
    }

    return sb1.st_dev == sb2.st_dev && sb1.st_ino == sb2.st_ino;
}
