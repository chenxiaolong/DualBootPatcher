/*
 * Copyright (C) 2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "wipe.h"

#include <algorithm>

#include <sys/stat.h>
#include <unistd.h>

#include "mbcommon/string.h"
#include "mblog/logging.h"
#include "mbutil/delete.h"
#include "mbutil/fts.h"
#include "mbutil/mount.h"
#include "mbutil/string.h"

#include "multiboot.h"

namespace mb
{

class WipeDirectory : public util::FTSWrapper {
public:
    WipeDirectory(std::string path, std::vector<std::string> exclusions)
        : FTSWrapper(path, FTS_GroupSpecialFiles),
        _exclusions(std::move(exclusions))
    {
    }

    virtual int on_changed_path() override
    {
        // Exclude first-level directories
        if (_curr->fts_level == 1) {
            if (std::find(_exclusions.begin(), _exclusions.end(), _curr->fts_name)
                    != _exclusions.end()) {
                return Action::FTS_Skip;
            }
        }

        return Action::FTS_OK;
    }

    virtual int on_reached_directory_pre() override
    {
        // Do nothing. Need depth-first search, so directories are deleted
        // in FTS_DP
        return Action::FTS_OK;
    }

    virtual int on_reached_directory_post() override
    {
        return delete_path() ? Action::FTS_OK : Action::FTS_Fail;
    }

    virtual int on_reached_file() override
    {
        return delete_path() ? Action::FTS_OK : Action::FTS_Fail;
    }

    virtual int on_reached_symlink() override
    {
        return delete_path() ? Action::FTS_OK : Action::FTS_Fail;
    }

    virtual int on_reached_special_file() override
    {
        return delete_path() ? Action::FTS_OK : Action::FTS_Fail;
    }

private:
    std::vector<std::string> _exclusions;

    bool delete_path()
    {
        if (_curr->fts_level >= 1 && remove(_curr->fts_accpath) < 0) {
            char *msg = mb_format("%s: Failed to remove: %s",
                                  _curr->fts_path, strerror(errno));
            if (msg) {
                _error_msg = msg;
                free(msg);
            }
            LOGW("%s", _error_msg.c_str());
            return false;
        }
        return true;
    }
};

bool wipe_directory(const std::string &directory,
                    const std::vector<std::string> &exclusions)
{
    struct stat sb;
    if (stat(directory.c_str(), &sb) < 0 && errno == ENOENT) {
        // Don't fail if directory does not exist
        return true;
    }

    std::vector<std::string> new_exclusions{ "multiboot" };
    new_exclusions.insert(new_exclusions.end(),
                          exclusions.begin(), exclusions.end());

    WipeDirectory wd(directory, std::move(new_exclusions));
    return wd.run();
}

/*!
 * \brief Log deletion of file
 *
 * \note The path will be deleted only if it is a regular file.
 *
 * \param path File to delete
 *
 * \return True if file was deleted or doesn't exist. False, otherwise.
 */
static bool log_wipe_file(const std::string &path)
{
    LOGV("Wiping file %s", path.c_str());

    struct stat sb;
    if (stat(path.c_str(), &sb) < 0) {
        if (errno != ENOENT) {
            LOGE("%s: Failed to stat: %s", path.c_str(), strerror(errno));
            return false;
        } else {
            return true;
        }
    }

    if (!S_ISREG(sb.st_mode)) {
        LOGE("%s: Cannot wipe file: not a regular file", path.c_str());
        return false;
    }

    bool ret = unlink(path.c_str()) == 0 || errno == ENOENT;
    LOGV("-> %s", ret ? "Succeeded" : "Failed");
    return ret;
}

/*!
 * \brief Log wiping of directory
 *
 * \note The path will be wiped only if it is a directory. The recursive
 *       deletion does not follow symlinks.
 *
 * \param mountpoint Mountpoint root to wipe
 * \param wipe_media Whether the first-level "media" path should be deleted
 *
 * \return True if the path was wiped or doesn't exist. False, otherwise
 */
static bool log_wipe_directory(const std::string &mountpoint,
                               const std::vector<std::string> &exclusions)
{
    if (exclusions.empty()) {
        LOGV("Wiping directory %s", mountpoint.c_str());
    } else {
        LOGV("Wiping directory %s (excluding %s)", mountpoint.c_str(),
             util::join(exclusions, ", ").c_str());
    }

    struct stat sb;
    if (stat(mountpoint.c_str(), &sb) < 0) {
        if (errno != ENOENT) {
            LOGE("%s: Failed to stat: %s", mountpoint.c_str(), strerror(errno));
            return false;
        } else {
            return true;
        }
    }

    if (!S_ISDIR(sb.st_mode)) {
        LOGE("%s: Cannot wipe path: not a directory", mountpoint.c_str());
        return false;
    }

    bool ret = wipe_directory(mountpoint, exclusions);
    LOGV("-> %s", ret ? "Succeeded" : "Failed");
    return ret;
}

static bool log_delete_recursive(const std::string &path)
{
    LOGV("Recursively deleting %s", path.c_str());
    bool ret = util::delete_recursive(path);
    LOGV("-> %s", ret ? "Succeeded" : "Failed");
    return ret;
}

bool wipe_system(const std::shared_ptr<Rom> &rom)
{
    std::string path = rom->full_system_path();
    if (path.empty()) {
        LOGE("Failed to determine full system path");
        return false;
    }

    bool ret;
    if (rom->system_is_image) {
        // Ensure the image is no longer mounted
        std::string mount_point("/raw/images/");
        mount_point += rom->id;
        util::umount(mount_point.c_str());

        ret = log_wipe_file(path);
    } else {
        ret = log_wipe_directory(path, {});
        // Try removing ROM's /system if it's empty
        remove(path.c_str());
    }
    return ret;
}

bool wipe_cache(const std::shared_ptr<Rom> &rom)
{
    std::string path = rom->full_cache_path();
    if (path.empty()) {
        LOGE("Failed to determine full cache path");
        return false;
    }

    bool ret;
    if (rom->cache_is_image) {
        ret = log_wipe_file(path);
    } else {
        ret = log_wipe_directory(path, {});
        // Try removing ROM's /cache if it's empty
        remove(path.c_str());
    }
    return ret;
}

bool wipe_data(const std::shared_ptr<Rom> &rom)
{
    std::string path = rom->full_data_path();
    if (path.empty()) {
        LOGE("Failed to determine full data path");
        return false;
    }

    bool ret;
    if (rom->data_is_image) {
        ret = log_wipe_file(path);
    } else {
        ret = log_wipe_directory(path, { "media" });
        // Try removing ROM's /data/media and /data if they're empty
        remove((path + "/media").c_str());
        remove(path.c_str());
    }
    return ret;
}

bool wipe_dalvik_cache(const std::shared_ptr<Rom> &rom)
{
    if (rom->data_is_image || rom->cache_is_image) {
        LOGE("Wiping dalvik-cache for ROMs that use data or cache images is "
             "currently not supported.");
        return false;
    }

    // Most ROMs use /data/dalvik-cache, but some use
    // /cache/dalvik-cache, like the jflte CyanogenMod builds
    std::string data_path = rom->full_data_path();
    std::string cache_path = rom->full_cache_path();

    if (data_path.empty()) {
        LOGE("Failed to determine full data path");
        return false;
    } else if (cache_path.empty()) {
        LOGE("Failed to determine full cache path");
        return false;
    }

    data_path += "/dalvik-cache";
    cache_path += "/dalvik-cache";

    // util::delete_recursive() returns true if the path does not
    // exist (ie. returns false only on errors), which is exactly
    // what we want
    return log_delete_recursive(data_path)
            && log_delete_recursive(cache_path);
}

bool wipe_multiboot(const std::shared_ptr<Rom> &rom)
{
    // Delete /data/media/0/MultiBoot/[ROM ID]
    std::string multiboot_path(MULTIBOOT_DIR);
    multiboot_path += '/';
    multiboot_path += rom->id;
    return log_delete_recursive(multiboot_path);
}

}
