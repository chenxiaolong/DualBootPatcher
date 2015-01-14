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

#include "multiboot.h"

#include <errno.h>
#include <fts.h>
#include <string.h>

#include "util/logging.h"


int mb_wipe_directory(const char *mountpoint, int wipeMedia)
{
    int ret = 0;
    FTS *ftsp = NULL;
    FTSENT *curr;

    char *files[] = { (char *) mountpoint, NULL };

    ftsp = fts_open(files, FTS_NOCHDIR | FTS_PHYSICAL | FTS_XDEV, NULL);
    if (!ftsp) {
        LOGE("%s: fts_open failed: %s", mountpoint, strerror(errno));
        ret = -1;
        goto finish;
    }

    while ((curr = fts_read(ftsp))) {
        switch (curr->fts_info) {
        case FTS_NS:
        case FTS_DNR:
        case FTS_ERR:
            LOGW("%s: fts_read error: %s",
                 curr->fts_accpath, strerror(curr->fts_errno));
            continue;

        case FTS_DC:
        case FTS_DOT:
        case FTS_NSOK:
            // Not reached
            continue;
        }

        if (curr->fts_level == 1) {
            if (strcmp(curr->fts_name, "multiboot") == 0
                    || (!wipeMedia && strcmp(curr->fts_name, "media") == 0)) {
                fts_set(ftsp, curr, FTS_SKIP);
                continue;
            }
        }

        switch (curr->fts_info) {
        case FTS_D:
            // Do nothing. Need depth-first search, so directories are deleted
            // in FTS_DP
            continue;

        case FTS_DP:
        case FTS_F:
        case FTS_SL:
        case FTS_SLNONE:
        case FTS_DEFAULT:
            if (curr->fts_level >= 1 && remove(curr->fts_accpath) < 0) {
                LOGW("%s: Failed to remove: %s",
                     curr->fts_path, strerror(errno));
                ret = -1;
            }
            continue;
        }
    }

finish:
    if (ftsp) {
        fts_close(ftsp);
    }

    return ret;
}
