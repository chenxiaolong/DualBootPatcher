/*
 * Copyright (C) 2014-2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
 *
 * This file is part of DualBootPatcher
 *
 * DualBootPatcher is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * DualBootPatcher is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with DualBootPatcher.  If not, see <http://www.gnu.org/licenses/>.
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
#include <sys/sysmacros.h>
#include <sys/vfs.h>
#include <unistd.h>

#include "mbcommon/error.h"
#include "mbcommon/finally.h"
#include "mbcommon/string.h"
#include "mblog/logging.h"
#include "mbutil/blkid.h"
#include "mbutil/directory.h"
#include "mbutil/loopdev.h"
#include "mbutil/string.h"

#define LOG_TAG "mbutil/mount"

#define MAX_UNMOUNT_TRIES 5

#define DELETED_SUFFIX " (deleted)"

namespace mb
{
namespace util
{

using ScopedFILE = std::unique_ptr<FILE, decltype(fclose) *>;

static std::string unescape_octals(const std::string &in)
{
    std::string result;

    for (auto it = in.begin(); it != in.end();) {
        if (*it == '\\' && std::distance(it, in.end()) >= 4
                && *(it + 1) >= '0' && *(it + 1) <= '7'
                && *(it + 2) >= '0' && *(it + 2) <= '7'
                && *(it + 3) >= '0' && *(it + 3) <= '7') {
            result += static_cast<char>(
                    ((*(it + 1) - '0') << 6)
                    | ((*(it + 2) - '0') << 3)
                    | (*(it + 3) - '0'));
            it += 4;
        } else {
            result += *it;
            ++it;
        }
    }

    return result;
}

bool get_mount_entry(std::FILE *fp, MountEntry &entry_out)
{
    char *line = nullptr;
    size_t size = 0;

    auto free_line = finally([&] {
        free(line);
    });

    if (getline(&line, &size, fp) < 0) {
        return false;
    }

    int pos[8];
    int count = std::sscanf(line, " %n%*s%n %n%*s%n %n%*s%n %n%*s%n %d %d",
            pos, pos + 1,       // fsname
            pos + 2, pos + 3,   // dir
            pos + 4, pos + 5,   // type
            pos + 6, pos + 7,   // opts
            &entry_out.freq,    // freq
            &entry_out.passno); // passno

    if (count != 2) {
        return false;
    }

    // NULL terminate entries
    line[pos[1]] = '\0';
    line[pos[3]] = '\0';
    line[pos[5]] = '\0';
    line[pos[7]] = '\0';

    entry_out.fsname = unescape_octals(line + pos[0]);
    entry_out.dir = unescape_octals(line + pos[2]);
    entry_out.type = unescape_octals(line + pos[4]);
    entry_out.opts = unescape_octals(line + pos[6]);

    struct stat sb;
    if (lstat(entry_out.dir.c_str(), &sb) < 0 && errno == ENOENT
            && ends_with(entry_out.dir, DELETED_SUFFIX)) {
        entry_out.dir.erase(entry_out.dir.size() - strlen(DELETED_SUFFIX));
    }

    return true;
}

bool is_mounted(const std::string &mountpoint)
{
    ScopedFILE fp(std::fopen(PROC_MOUNTS, "r"), std::fclose);
    if (!fp) {
        LOGE("%s: Failed to read file: %s", PROC_MOUNTS, strerror(errno));
        return false;
    }

    for (MountEntry entry; get_mount_entry(fp.get(), entry);) {
        if (mountpoint == entry.dir) {
            return true;
        }
    }

    return false;
}

bool unmount_all(const std::string &dir)
{
    std::vector<std::string> to_unmount;
    int failed;

    for (int tries = 0; tries < MAX_UNMOUNT_TRIES; ++tries) {
        failed = 0;
        to_unmount.clear();

        ScopedFILE fp(std::fopen(PROC_MOUNTS, "r"), std::fclose);
        if (!fp) {
            LOGE("%s: Failed to read file: %s", PROC_MOUNTS, strerror(errno));
            return false;
        }

        for (MountEntry entry; get_mount_entry(fp.get(), entry);) {
            // TODO: Use path_compare() instead of dumb string prefix matching
            if (starts_with(entry.dir, dir)) {
                to_unmount.push_back(std::move(entry.dir));
            }
        }

        // Unmount in reverse order
        for (auto it = to_unmount.rbegin(); it != to_unmount.rend(); ++it) {
            LOGD("Attempting to unmount %s", it->c_str());

            if (!umount(*it)) {
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
bool mount(const std::string &source, const std::string &target,
           const std::string &fstype, unsigned long mount_flags,
           const std::string &data)
{
    bool need_loopdev = false;
    struct stat sb;
    std::string fstype_real{fstype};

    if (!(mount_flags & (MS_REMOUNT | MS_BIND | MS_MOVE))) {
        if (stat(source.c_str(), &sb) >= 0) {
            if (S_ISREG(sb.st_mode)) {
                need_loopdev = true;
            }
            if (fstype == "auto"
                    && (S_ISREG(sb.st_mode) || S_ISBLK(sb.st_mode))) {
                optional<std::string> detected;
                if (!blkid_get_fs_type(source, detected) || !detected) {
                    return false;
                } else if (*detected == "ext") {
                    // Always assume ext4 instead of ext2, ext3, ext4dev, or jbd
                    fstype_real = "ext4";
                } else {
                    fstype_real.swap(*detected);
                }
            }
        }
    }

    if (need_loopdev) {
        std::string loopdev = loopdev_find_unused();
        if (loopdev.empty()) {
            LOGE("Failed to find unused loop device: %s", strerror(errno));
            return false;
        }

        LOGD("Assigning %s to loop device %s", source.c_str(), loopdev.c_str());

        if (!loopdev_set_up_device(
                loopdev, source, 0, mount_flags & MS_RDONLY)) {
            LOGE("Failed to set up loop device %s: %s",
                 loopdev.c_str(), strerror(errno));
            return false;
        }

        if (::mount(loopdev.c_str(), target.c_str(), fstype_real.c_str(),
                    mount_flags, data.c_str()) < 0) {
            loopdev_remove_device(loopdev);
            return false;
        }

        return true;
    } else {
        return ::mount(source.c_str(), target.c_str(), fstype_real.c_str(),
                       mount_flags, data.c_str()) == 0;
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
bool umount(const std::string &target)
{
    std::string source;
    std::string mnt_dir;

    ScopedFILE fp(std::fopen(PROC_MOUNTS, "r"), std::fclose);
    if (fp) {
        for (MountEntry entry; get_mount_entry(fp.get(), entry);) {
            if (entry.dir == target) {
                source = entry.fsname;
            }
        }
    } else {
        LOGW("%s: Failed to read file: %s", PROC_MOUNTS, strerror(errno));
    }

    int ret = ::umount(target.c_str());

    ErrorRestorer restorer;

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

    return ret == 0;
}

bool mount_get_total_size(const std::string &path, uint64_t &size)
{
    struct statfs sfs;

    if (statfs(path.c_str(), &sfs) < 0) {
        return false;
    }

    size = sfs.f_bsize * sfs.f_blocks;
    return true;
}

bool mount_get_avail_size(const std::string &path, uint64_t &size)
{
    struct statfs sfs;

    if (statfs(path.c_str(), &sfs) < 0) {
        return false;
    }

    size = sfs.f_bsize * sfs.f_bavail;
    return true;
}

}
}
