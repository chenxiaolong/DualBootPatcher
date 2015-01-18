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

#include "util/mount.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <unistd.h>

#include "external/mntent.h"
#include "logging.h"
#include "util/directory.h"

#define MAX_UNMOUNT_TRIES 5

int mb_is_mounted(const char *mountpoint)
{
    FILE *f;
    int ret = 0;
    struct mntent ent;
    char buf[1024];

    f = setmntent("/proc/mounts", "r");
    if (f != NULL) {
        while (getmntent_r(f, &ent, buf, sizeof(buf))) {
            if (strcmp(mountpoint, ent.mnt_dir) == 0) {
                ret = 1;
                break;
            }
        }
        endmntent(f);
    }

    return ret;
}

int mb_unmount_all(const char *dir)
{
    FILE *fp;
    int failed;
    struct mntent ent;
    char buf[1024];

    for (int tries = 0; tries < MAX_UNMOUNT_TRIES; ++tries) {
        failed = 0;

        fp = setmntent("/proc/mounts", "r");
        if (fp) {
            while (getmntent_r(fp, &ent, buf, sizeof(buf))) {
                if (strlen(ent.mnt_dir) >= strlen(dir)
                        && strncmp(ent.mnt_dir, dir, strlen(dir)) == 0) {
                    //LOGD("Attempting to unmount %s", ent.mnt_dir);

                    if (umount(ent.mnt_dir) < 0) {
                        LOGE("Failed to unmount %s: %s",
                             ent.mnt_dir, strerror(errno));
                        ++failed;
                    }
                }
            }
            endmntent(fp);
        } else {
            LOGE("Failed to read /proc/mounts: %s", strerror(errno));
            return -1;
        }

        if (failed == 0) {
            return 0;
        }

        // Retry
    }

    LOGE("Failed to unmount %d partitions", failed);
    return -1;
}

int mb_bind_mount(const char *source, mode_t source_perms,
                  const char *target, mode_t target_perms)
{
    struct stat sb;

    if (stat(source, &sb) < 0 && mb_mkdir_recursive(source, source_perms) < 0) {
        LOGE("Failed to create %s", source);
        return -1;
    }

    if (stat(target, &sb) < 0 && mb_mkdir_recursive(target, target_perms) < 0) {
        LOGE("Failed to create %s", target);
        return -1;
    }

    if (chmod(source, source_perms) < 0) {
        LOGE("Failed to chmod %s", source);
        return -1;
    }

    if (chmod(target, target_perms) < 0) {
        LOGE("Failed to chmod %s", target);
        return -1;
    }

    if (mount(source, target, "", MS_BIND, "") < 0) {
        LOGE("Failed to bind mount %s to %s: %s",
             source, target, strerror(errno));
        return -1;
    }

    return 0;
}

int64_t mb_mount_get_total_size(const char *mountpoint)
{
    struct statfs sfs;

    if (statfs(mountpoint, &sfs) < 0) {
        return 0;
    }

    return sfs.f_bsize * sfs.f_blocks;
}

int64_t mb_mount_get_avail_size(const char *mountpoint)
{
    struct statfs sfs;

    if (statfs(mountpoint, &sfs) < 0) {
        return 0;
    }

    return sfs.f_bsize * sfs.f_bavail;
}
