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

#include "util/chown.h"

#include <errno.h>
#include <fts.h>
#include <grp.h>
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>

#include "util/logging.h"

// WARNING: Not thread safe! Android doesn't have getpwnam_r() or getgrnam_r()
static int mb_chown_internal(const char *path, const char *user,
                             const char *group, int follow_symlinks)
{
    uid_t uid;
    gid_t gid;

    errno = 0;
    struct passwd *pw = getpwnam(user);
    if (!pw) {
        if (!errno) {
            errno = EINVAL; // User does not exist
        }
        return -1;
    } else {
        uid = pw->pw_uid;
    }

    errno = 0;
    struct group *gr = getgrnam(group);
    if (!gr) {
        if (!errno) {
            errno = EINVAL; // Group does not exist
        }
        return -1;
    } else {
        gid = gr->gr_gid;
    }

    return (follow_symlinks ? chown : lchown)(path, uid, gid);
}

static int mb_chown_internal_recursive(const char *path, const char *user,
                                       const char *group, int follow_symlinks)
{
    int ret = 0;
    FTS *ftsp = NULL;
    FTSENT *curr;

    char *files[] = { (char *) path, NULL };

    ftsp = fts_open(files, FTS_NOCHDIR | FTS_PHYSICAL | FTS_XDEV, NULL);
    if (!ftsp) {
        LOGE("%s: fts_open failed: %s", path, strerror(errno));
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
            break;

        case FTS_DC:
        case FTS_DOT:
        case FTS_NSOK:
            // Not reached
            break;

        case FTS_D:
            // Do nothing. Need depth-first search, so directories are deleted
            // in FTS_DP
            break;

        case FTS_DP:
        case FTS_F:
        case FTS_SL:
        case FTS_SLNONE:
        case FTS_DEFAULT:
            if (mb_chown_internal(curr->fts_accpath, user, group, follow_symlinks) < 0) {
                LOGW("%s: Failed to chown: %s",
                     curr->fts_path, strerror(errno));
                ret = -1;
            }
            break;
        }
    }

finish:
    if (ftsp) {
        fts_close(ftsp);
    }

    return ret;
}

int mb_chown(const char *path, const char *user, const char *group, int flags)
{
    if (flags & MB_CHOWN_RECURSIVE) {
        return mb_chown_internal_recursive(path, user, group,
                                           flags & MB_CHOWN_FOLLOW_SYMLINKS);
    } else {
        return mb_chown_internal(path, user, group,
                                 flags & MB_CHOWN_FOLLOW_SYMLINKS);
    }
}
