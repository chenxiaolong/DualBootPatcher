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

#include "util/delete.h"

#include <errno.h>
#include <ftw.h>
#include <stdio.h>

#include "logging.h"

static int nftw_delete_cb(const char *fpath, const struct stat *sb,
                          int typeflag, struct FTW *ftwbuf)
{
    (void) sb;
    (void) typeflag;
    (void) ftwbuf;

    int ret;
    if ((ret = remove(fpath)) < 0) {
        LOGE("Failed to remove: %s", fpath);
    }

    return ret;
}

int mb_delete_recursive(const char *path)
{
    struct stat sb;
    if (stat(path, &sb) < 0 && errno == ENOENT) {
        // Don't fail if directory does not exist
        return 0;
    }

    // Recursively delete directory without crossing mountpoint boundaries
    return nftw(path, nftw_delete_cb, 64,
                FTW_DEPTH | FTW_MOUNT | FTW_PHYS);
}
