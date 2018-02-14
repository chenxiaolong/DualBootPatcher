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

#include "appsyncmanager.h"

#include <algorithm>

#include <cerrno>

#include <sys/mount.h>
#include <sys/stat.h>
#include <unistd.h>

#include "mblog/logging.h"
#include "mbutil/chown.h"
#include "mbutil/directory.h"
#include "mbutil/fts.h"
#include "mbutil/selinux.h"

#define LOG_TAG "mbtool/appsyncmanager"

#define APP_SHARING_DATA_DIR            "/data/multiboot/_appsharing/data"

#define USER_DATA_DIR                   "/data/data"

static std::string _as_data_dir;
static std::string _user_data_dir;

namespace mb
{

/*!
 * Recursively chmod directories to 755 and files to 0644 and chown everything
 * system:system.
 */
class FixPermissions : public util::FtsWrapper {
public:
    FixPermissions(std::string path)
        : FtsWrapper(path, util::FtsFlag::GroupSpecialFiles)
    {
    }

    Actions on_changed_path() override
    {
        (void) util::chown(_curr->fts_accpath, "system", "system", 0);
        return Action::Ok;
    }

    Actions on_reached_file() override
    {
        chmod(_curr->fts_accpath, 0644);
        return Action::Ok;
    }

    Actions on_reached_directory_pre() override
    {
        chmod(_curr->fts_accpath, 0755);
        return Action::Ok;
    }
};

void AppSyncManager::detect_directories()
{
    _as_data_dir = get_raw_path(APP_SHARING_DATA_DIR);
    _user_data_dir = USER_DATA_DIR;

    LOGD("App sharing app data directory: %s", _as_data_dir.c_str());
    LOGD("User app data directory:        %s", _user_data_dir.c_str());
}

/*!
 * \brief Get shared data path for a package
 */
std::string AppSyncManager::get_shared_data_path(const std::string &pkg)
{
    std::string path(_as_data_dir);
    path += "/";
    path += pkg;
    return path;
}

bool AppSyncManager::initialize_directories()
{
    if (auto r = util::mkdir_recursive(_as_data_dir, 0751);
            !r && r.error() != std::errc::file_exists) {
        LOGW("%s: Failed to create directory: %s", _as_data_dir.c_str(),
             r.error().message().c_str());
        return false;
    }

    return true;
}

bool AppSyncManager::create_shared_data_directory(const std::string &pkg, uid_t uid)
{
    std::string data_path = get_shared_data_path(pkg);

    if (auto r = util::mkdir_recursive(data_path, 0751); !r) {
        LOGW("[%s] %s: Failed to create directory: %s",
             pkg.c_str(), data_path.c_str(), r.error().message().c_str());
        return false;
    }

    // Ensure that the shared data directory permissions are correct
    if (chmod(data_path.c_str(), 0751) < 0) {
        LOGW("[%s] %s: Failed to chmod: %s",
             pkg.c_str(), data_path.c_str(), strerror(errno));
        return false;
    }

    if (auto r = util::chown(
            data_path, uid, uid, util::ChownFlag::Recursive); !r) {
        LOGW("[%s] %s: Failed to chown: %s",
             pkg.c_str(), data_path.c_str(), r.error().message().c_str());
        return false;
    }

    return true;
}

bool AppSyncManager::fix_shared_data_permissions()
{
    std::string context("u:object_r:app_data_file:s0");
    if (auto ret = util::selinux_lget_context(
            "/data/data/com.android.systemui")) {
        context.swap(ret.value());
    }

    if (auto ret = util::selinux_lset_context_recursive(
            _as_data_dir, context); !ret) {
        LOGW("%s: Failed to set context recursively to %s: %s",
             _as_data_dir.c_str(), context.c_str(),
             ret.error().message().c_str());
        return false;
    }

    return true;
}

bool AppSyncManager::mount_shared_directory(const std::string &pkg, uid_t uid)
{
    std::string data_path = get_shared_data_path(pkg);
    std::string target(_user_data_dir);
    target += "/";
    target += pkg;

    if (auto r = util::mkdir_recursive(target, 0755); !r) {
        LOGW("[%s] %s: Failed to create directory: %s",
             pkg.c_str(), target.c_str(), r.error().message().c_str());
        return false;
    }
    if (auto r = util::chown(target, uid, uid, util::ChownFlag::Recursive); !r) {
        LOGW("[%s] %s: Failed to chown: %s",
             pkg.c_str(), target.c_str(), r.error().message().c_str());
        return false;
    }

    LOGV("[%s] Bind mounting data directory:", pkg.c_str());
    LOGV("[%s] - Source: %s", pkg.c_str(), data_path.c_str());
    LOGV("[%s] - Target: %s", pkg.c_str(), target.c_str());

    if (!unmount_shared_directory(pkg)) {
        return false;
    } else if (mount(data_path.c_str(), target.c_str(), "", MS_BIND, "") < 0) {
        LOGW("[%s] Failed to bind mount: %s", pkg.c_str(), strerror(errno));
        return false;
    }

    return true;
}

bool AppSyncManager::unmount_shared_directory(const std::string &pkg)
{
    std::string target(_user_data_dir);
    target += "/";
    target += pkg;

    if (umount2(target.c_str(), MNT_DETACH) < 0 && errno != EINVAL) {
        LOGW("[%s] %s: Failed to unmount: %s",
             pkg.c_str(), target.c_str(), strerror(errno));
        return false;
    }

    return true;
}

}
