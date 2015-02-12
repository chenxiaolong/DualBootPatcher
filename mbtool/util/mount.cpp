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

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <unistd.h>

#include "external/mntent.h"
#include "logging.h"
#include "util/directory.h"
#include "util/string.h"

#define MAX_UNMOUNT_TRIES 5

namespace mb
{
namespace util
{

typedef std::unique_ptr<std::FILE, int (*)(std::FILE *)> file_ptr;

bool is_mounted(const std::string &mountpoint)
{
    file_ptr fp(setmntent("/proc/mounts", "r"), endmntent);
    if (!fp) {
        return false;
    }

    bool found = false;
    struct mntent ent;
    char buf[1024];

    while (getmntent_r(fp.get(), &ent, buf, sizeof(buf))) {
        if (mountpoint == ent.mnt_dir) {
            found = true;
            break;
        }
    }

    return found;
}

bool unmount_all(const std::string &dir)
{
    int failed;
    struct mntent ent;
    char buf[1024];

    for (int tries = 0; tries < MAX_UNMOUNT_TRIES; ++tries) {
        failed = 0;

        file_ptr fp(setmntent("/proc/mounts", "r"), endmntent);
        if (!fp) {
            LOGE("Failed to read /proc/mounts: %s", strerror(errno));
            return false;
        }

        while (getmntent_r(fp.get(), &ent, buf, sizeof(buf))) {
            if (starts_with(ent.mnt_dir, dir)) {
                //LOGD("Attempting to unmount %s", ent.mnt_dir);

                if (umount(ent.mnt_dir) < 0) {
                    LOGE("Failed to unmount %s: %s",
                          ent.mnt_dir, strerror(errno));
                    ++failed;
                }
            }
        }

        if (failed == 0) {
            return true;
        }

        // Retry
    }

    LOGE("Failed to unmount %d partitions", failed);
    return false;
}

bool bind_mount(const std::string &source, mode_t source_perms,
                const std::string &target, mode_t target_perms)
{
    struct stat sb;

    if (stat(source.c_str(), &sb) < 0
            && !mkdir_recursive(source, source_perms)) {
        LOGE("Failed to create %s", source);
        return false;
    }

    if (stat(target.c_str(), &sb) < 0
            && !mkdir_recursive(target, target_perms)) {
        LOGE("Failed to create %s", target);
        return false;
    }

    if (chmod(source.c_str(), source_perms) < 0) {
        LOGE("Failed to chmod %s", source);
        return false;
    }

    if (chmod(target.c_str(), target_perms) < 0) {
        LOGE("Failed to chmod %s", target);
        return false;
    }

    if (mount(source.c_str(), target.c_str(), "", MS_BIND, "") < 0) {
        LOGE("Failed to bind mount %s to %s: %s",
             source, target, strerror(errno));
        return false;
    }

    return true;
}

int64_t mount_get_total_size(const std::string &mountpoint)
{
    struct statfs sfs;

    if (statfs(mountpoint.c_str(), &sfs) < 0) {
        return 0;
    }

    return sfs.f_bsize * sfs.f_blocks;
}

int64_t mount_get_avail_size(const std::string &mountpoint)
{
    struct statfs sfs;

    if (statfs(mountpoint.c_str(), &sfs) < 0) {
        return 0;
    }

    return sfs.f_bsize * sfs.f_bavail;
}

}
}