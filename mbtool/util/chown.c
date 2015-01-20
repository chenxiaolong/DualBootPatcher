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
#include <grp.h>
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>

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

int mb_chown_name(const char *path, const char *user, const char *group)
{
    return mb_chown_internal(path, user, group, 1);
}

int mb_lchown_name(const char *path, const char *user, const char *group)
{
    return mb_chown_internal(path, user, group, 0);
}
