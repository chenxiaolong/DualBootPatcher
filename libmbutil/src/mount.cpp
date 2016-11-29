/*
 * Copyright (C) 2014-2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbutil/mount.h"

#include <memory>
#include <vector>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <unistd.h>

#include "mblog/logging.h"
#include "mbutil/autoclose/file.h"
#include "mbutil/blkid.h"
#include "mbutil/directory.h"
#include "mbutil/loopdev.h"
#include "mbutil/string.h"
#include "external/mntent.h"

#define MAX_UNMOUNT_TRIES 5

#define DELETED_SUFFIX "\\040(deleted)"

namespace mb
{
namespace util
{

static std::string get_deleted_mount_path(const std::string &dir)
{
    struct stat sb;
    if (lstat(dir.c_str(), &sb) < 0 && errno == ENOENT
            && util::ends_with(dir, DELETED_SUFFIX)) {
        return std::string(dir.begin(), dir.end() - strlen(DELETED_SUFFIX));
    } else {
        return dir;
    }
}

bool is_mounted(const std::string &mountpoint)
{
    autoclose::file fp(setmntent("/proc/mounts", "r"), endmntent);
    if (!fp) {
        LOGE("Failed to read /proc/mounts: %s", strerror(errno));
        return false;
    }

    bool found = false;
    struct mntent ent;
    char buf[1024];
    std::string mnt_dir;

    while (getmntent_r(fp.get(), &ent, buf, sizeof(buf))) {
        mnt_dir = get_deleted_mount_path(ent.mnt_dir);
        if (mountpoint == mnt_dir) {
            found = true;
            break;
        }
    }

    return found;
}

bool unmount_all(const std::string &dir)
{
    std::vector<std::string> to_unmount;
    int failed;
    struct mntent ent;
    char buf[1024];

    for (int tries = 0; tries < MAX_UNMOUNT_TRIES; ++tries) {
        failed = 0;
        to_unmount.clear();

        autoclose::file fp(setmntent("/proc/mounts", "r"), endmntent);
        if (!fp) {
            LOGE("Failed to read /proc/mounts: %s", strerror(errno));
            return false;
        }

        while (getmntent_r(fp.get(), &ent, buf, sizeof(buf))) {
            std::string mnt_dir = get_deleted_mount_path(ent.mnt_dir);
            if (starts_with(mnt_dir, dir)) {
                to_unmount.push_back(std::move(mnt_dir));
            }
        }

        // Unmount in reverse order
        for (auto it = to_unmount.rbegin(); it != to_unmount.rend(); ++it) {
            LOGD("Attempting to unmount %s", it->c_str());

            if (!util::umount(it->c_str())) {
                LOGE("%s: Failed to unmount: %s",
                     it->c_str(), strerror(errno));
                ++failed;
            }
        }

        // No more matching mount points
        if (failed == 0) {
            return true;
        }

        // Retry
    }

    LOGE("Failed to unmount %d mount points", failed);
    return false;
}

/*!
 * \brief Bind mount a directory
 *
 * This function will create or chmod the source and target directories before
 * performing the bind mount. If the source or target directories don't exist,
 * they will be created (recursively) with the specified permissions. If the
 * directories already exist, they will be chmod'ed with the specified mode
 * (parent directories will not be touched).
 *
 * \param source Source path
 * \param source_perms Permissions for source path
 * \param target Target path
 * \param target_perms Permissions for target path
 *
 * \return True if bind mount is successful. False, otherwise.
 */
bool bind_mount(const std::string &source, mode_t source_perms,
                const std::string &target, mode_t target_perms)
{
    struct stat sb;

    if (stat(source.c_str(), &sb) < 0
            && !mkdir_recursive(source, source_perms)) {
        LOGE("Failed to create %s: %s", source.c_str(), strerror(errno));
        return false;
    }

    if (stat(target.c_str(), &sb) < 0
            && !mkdir_recursive(target, target_perms)) {
        LOGE("Failed to create %s: %s", target.c_str(), strerror(errno));
        return false;
    }

    if (chmod(source.c_str(), source_perms) < 0) {
        LOGE("Failed to chmod %s: %s", source.c_str(), strerror(errno));
        return false;
    }

    if (chmod(target.c_str(), target_perms) < 0) {
        LOGE("Failed to chmod %s: %s", target.c_str(), strerror(errno));
        return false;
    }

    if (::mount(source.c_str(), target.c_str(), "", MS_BIND, "") < 0) {
        LOGE("Failed to bind mount %s to %s: %s",
             source.c_str(), target.c_str(), strerror(errno));
        return false;
    }

    return true;
}

/*!
 * \brief Mount filesystem
 *
 * This function takes the same arguments as mount(2), but returns true on
 * success and false on failure.
 *
 * If MS_BIND is not specified in \a mount_flags and \a source is not a block
 * device, then the file will be attached to a loop device and the the loop
 * device will be mounted at \a target.
 *
 * \param source See man mount(2)
 * \param target See man mount(2)
 * \param fstype See man mount(2)
 * \param mount_flags See man mount(2)
 * \param data See man mount(2)
 *
 * \return True if mount(2) is successful. False if mount(2) is unsuccessful
 *         or loopdev could not be created or associated with the source path.
 */
bool mount(const char *source, const char *target, const char *fstype,
           unsigned long mount_flags, const void *data)
{
    bool need_loopdev = false;
    struct stat sb;

    if (!(mount_flags & (MS_REMOUNT | MS_BIND | MS_MOVE))) {
        if (stat(source, &sb) >= 0) {
            if (S_ISREG(sb.st_mode)) {
                need_loopdev = true;
            }
            if (fstype && strcmp(fstype, "auto") == 0
                    && (S_ISREG(sb.st_mode) || S_ISBLK(sb.st_mode))) {
                if (!blkid_get_fs_type(source, &fstype) || !fstype) {
                    return false;
                } else if (strcmp(fstype, "ext") == 0) {
                    // Always assume ext4 instead of ext2, ext3, ext4dev, or jbd
                    fstype = "ext4";
                }
            }
        }
    }

    if (need_loopdev) {
        std::string loopdev = util::loopdev_find_unused();
        if (loopdev.empty()) {
            LOGE("Failed to find unused loop device: %s", strerror(errno));
            return false;
        }

        LOGD("Assigning %s to loop device %s", source, loopdev.c_str());

        if (!util::loopdev_set_up_device(
                loopdev, source, 0, mount_flags & MS_RDONLY)) {
            LOGE("Failed to set up loop device %s: %s",
                 loopdev.c_str(), strerror(errno));
            return false;
        }

        if (::mount(loopdev.c_str(), target, fstype, mount_flags, data) < 0) {
            util::loopdev_remove_device(loopdev);
            return false;
        }

        return true;
    } else {
        return ::mount(source, target, fstype, mount_flags, data) == 0;
    }
}

/*!
 * \brief Unmount filesystem
 *
 * This function takes the same arguments as umount(2), but returns true on
 * success and false on failure.
 *
 * This function will /proc/mounts for the mountpoint (using an exact string
 * compare). If the source path of the mountpoint is a block device and the
 * block device is a loop device, then it will be disassociated from the
 * previously attached file. Note that the return value of
 * loopdev_remove_device() is ignored and this function will always return true
 * if umount(2) is successful.
 *
 * \param target See man umount(2)
 *
 * \return True if umount(2) is successful. False if umount(2) is unsuccessful.
 */
bool umount(const char *target)
{
    struct mntent ent;
    std::string source;
    std::string mnt_dir;

    autoclose::file fp(setmntent("/proc/mounts", "r"), endmntent);
    if (fp) {
        char buf[1024];
        while (getmntent_r(fp.get(), &ent, buf, sizeof(buf))) {
            mnt_dir = get_deleted_mount_path(ent.mnt_dir);
            if (mnt_dir == target) {
                source = ent.mnt_fsname;
            }
        }
    } else {
        LOGW("Failed to read /proc/mounts: %s", strerror(errno));
    }

    int ret = ::umount(target);

    int saved_errno = errno;

    if (!source.empty()) {
        struct stat sb;

        if (stat(source.c_str(), &sb) == 0
                && S_ISBLK(sb.st_mode) && major(sb.st_rdev) == 7) {
            // If the source path is a loop block device, then disassociate it
            // from the image
            LOGD("Clearing loop device %s", source.c_str());
            if (!loopdev_remove_device(source)) {
                LOGW("Failed to clear loop device: %s", strerror(errno));
            }
        }
    }

    errno = saved_errno;

    return ret == 0;
}

uint64_t mount_get_total_size(const char *path)
{
    struct statfs sfs;

    if (statfs(path, &sfs) < 0) {
        return 0;
    }

    return sfs.f_bsize * sfs.f_blocks;
}

uint64_t mount_get_avail_size(const char *path)
{
    struct statfs sfs;

    if (statfs(path, &sfs) < 0) {
        return 0;
    }

    return sfs.f_bsize * sfs.f_bavail;
}

}
}
