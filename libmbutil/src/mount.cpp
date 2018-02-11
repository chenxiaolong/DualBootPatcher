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

#include "mbcommon/error_code.h"
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

namespace mb::util
{

using ScopedFILE = std::unique_ptr<FILE, decltype(fclose) *>;

struct MountErrorCategory : std::error_category
{
    const char * name() const noexcept override;

    std::string message(int ev) const override;
};

const std::error_category & mount_error_category()
{
    static MountErrorCategory c;
    return c;
}

std::error_code make_error_code(MountError e)
{
    return {static_cast<int>(e), mount_error_category()};
}

const char * MountErrorCategory::name() const noexcept
{
    return "mount";
}

std::string MountErrorCategory::message(int ev) const
{
    switch (static_cast<MountError>(ev)) {
    case MountError::EndOfFile:
        return "end of file";
    case MountError::PathNotMounted:
        return "path not mounted";
    default:
        return "(unknown mount error)";
    }
}

static std::string unescape_octals(std::string_view in)
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

oc::result<MountEntry> get_mount_entry(std::FILE *fp)
{
    MountEntry entry;
    char *line = nullptr;
    size_t size = 0;

    auto free_line = finally([&] {
        free(line);
    });

    ssize_t n = getline(&line, &size, fp);
    if (n < 0) {
        if (ferror(fp)) {
            return ec_from_errno();
        } else {
            return MountError::EndOfFile;
        }
    }

    int pos[8];
    int count = std::sscanf(line, " %n%*s%n %n%*s%n %n%*s%n %n%*s%n %d %d",
            pos, pos + 1,       // fsname
            pos + 2, pos + 3,   // dir
            pos + 4, pos + 5,   // type
            pos + 6, pos + 7,   // opts
            &entry.freq,        // freq
            &entry.passno);     // passno

    if (count != 2) {
        return std::errc::invalid_argument;
    }

    // NULL terminate entries
    line[pos[1]] = '\0';
    line[pos[3]] = '\0';
    line[pos[5]] = '\0';
    line[pos[7]] = '\0';

    entry.fsname = unescape_octals(line + pos[0]);
    entry.dir = unescape_octals(line + pos[2]);
    entry.type = unescape_octals(line + pos[4]);
    entry.opts = unescape_octals(line + pos[6]);

    struct stat sb;
    if (lstat(entry.dir.c_str(), &sb) < 0 && errno == ENOENT
            && ends_with(entry.dir, DELETED_SUFFIX)) {
        entry.dir.erase(entry.dir.size() - strlen(DELETED_SUFFIX));
    }

    return std::move(entry);
}

oc::result<void> is_mounted(const std::string &mountpoint)
{
    ScopedFILE fp(std::fopen(PROC_MOUNTS, "r"), std::fclose);
    if (!fp) {
        return ec_from_errno();
    }

    while (true) {
        auto entry = get_mount_entry(fp.get());
        if (!entry) {
            if (entry.error() == MountError::EndOfFile) {
                break;
            } else {
                return entry.as_failure();
            }
        } else if (entry.value().dir == mountpoint) {
            return oc::success();
        }
    }

    return MountError::PathNotMounted;
}

oc::result<void> unmount_all(const std::string &dir)
{
    std::vector<std::string> to_unmount;
    int failed = 0;
    std::error_code ec;

    for (int tries = 0; tries < MAX_UNMOUNT_TRIES; ++tries) {
        failed = 0;
        to_unmount.clear();
        ec.clear();

        ScopedFILE fp(std::fopen(PROC_MOUNTS, "r"), std::fclose);
        if (!fp) {
            return ec_from_errno();
        }

        while (auto entry = get_mount_entry(fp.get())) {
            // TODO: Use path_compare() instead of dumb string prefix matching
            if (starts_with(entry.value().dir, dir)) {
                to_unmount.push_back(std::move(entry.value().dir));
            }
        }

        // Unmount in reverse order
        for (auto it = to_unmount.rbegin(); it != to_unmount.rend(); ++it) {
            LOGD("Attempting to unmount %s", it->c_str());

            if (auto ret = umount(*it); !ret) {
                LOGW("%s: Failed to unmount: %s",
                     it->c_str(), ret.error().message().c_str());
                ++failed;
                ec = ret.error();
            }
        }

        // No more matching mount points
        if (failed == 0) {
            return oc::success();
        }

        // Retry
    }

    LOGW("Failed to unmount %d mount points", failed);
    return ec;
}

/*!
 * \brief Mount filesystem
 *
 * This function takes the same arguments as mount(2), but returns nothing on
 * success and the error on failure.
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
 * \return Nothing if mount(2) is successful. The error code if mount(2) is
 *         unsuccessful or the loopdev could not be created or be associated
 *         with the source path.
 */
oc::result<void> mount(const std::string &source, const std::string &target,
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
                OUTCOME_TRY(detected, blkid_get_fs_type(source));
                if (detected.empty()) {
                    return std::errc::invalid_argument;
                } else if (detected == "ext") {
                    // Always assume ext4 instead of ext2, ext3, ext4dev, or jbd
                    fstype_real = "ext4";
                } else {
                    fstype_real.swap(detected);
                }
            }
        }
    }

    if (need_loopdev) {
        OUTCOME_TRY(loopdev, loopdev_find_unused());
        OUTCOME_TRYV(loopdev_set_up_device(loopdev, source, 0,
                                           mount_flags & MS_RDONLY));

        if (::mount(loopdev.c_str(), target.c_str(), fstype_real.c_str(),
                    mount_flags, data.c_str()) < 0) {
            int saved_errno = errno;
            (void) loopdev_remove_device(loopdev);
            return ec_from_errno(saved_errno);
        }
    } else {
        if (::mount(source.c_str(), target.c_str(), fstype_real.c_str(),
                    mount_flags, data.c_str()) < 0) {
            return ec_from_errno();
        }
    }

    return oc::success();
}

/*!
 * \brief Unmount filesystem
 *
 * This function takes the same arguments as umount(2), but returns nothing on
 * success and the error on failure.
 *
 * This function will search /proc/mounts for the mountpoint (using an exact
 * string compare). If the source path of the mountpoint is a block device and
 * the block device is a loop device, then it will be disassociated from the
 * previously attached file. Note that the return value of
 * loopdev_remove_device() is ignored and this function will always return a
 * success result if umount(2) is successful.
 *
 * \param target See `man umount(2)`
 *
 * \return Nothing if umount(2) is successful. Otherwise, the error code.
 */
oc::result<void> umount(const std::string &target)
{
    std::string source;
    std::string mnt_dir;

    ScopedFILE fp(std::fopen(PROC_MOUNTS, "r"), std::fclose);
    if (fp) {
        while (auto entry = get_mount_entry(fp.get())) {
            if (entry.value().dir == target) {
                source = entry.value().fsname;
            }
        }
    } else {
        LOGW("%s: Failed to read file: %s", PROC_MOUNTS, strerror(errno));
    }

    oc::result<void> ret = oc::success();
    if (::umount(target.c_str()) < 0) {
        ret = ec_from_errno();
    }

    if (!source.empty()) {
        struct stat sb;

        if (stat(source.c_str(), &sb) == 0
                && S_ISBLK(sb.st_mode) && major(sb.st_rdev) == 7) {
            // If the source path is a loop block device, then disassociate it
            // from the image
            LOGD("Clearing loop device %s", source.c_str());
            if (auto r = loopdev_remove_device(source); !r) {
                LOGW("Failed to clear loop device: %s",
                     ret.error().message().c_str());
            }
        }
    }

    return ret;
}

oc::result<uint64_t> mount_get_total_size(const std::string &path)
{
    struct statfs sfs;

    if (statfs(path.c_str(), &sfs) < 0) {
        return ec_from_errno();
    }

    return sfs.f_bsize * sfs.f_blocks;
}

oc::result<uint64_t> mount_get_avail_size(const std::string &path)
{
    struct statfs sfs;

    if (statfs(path.c_str(), &sfs) < 0) {
        return ec_from_errno();
    }

    return sfs.f_bsize * sfs.f_bavail;
}

}
