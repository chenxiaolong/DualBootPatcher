/*
 * Copyright (C) 2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "appsyncmanager.h"

#include <cerrno>

#include <sys/mount.h>
#include <sys/stat.h>
#include <unistd.h>

#include "util/chown.h"
#include "util/copy.h"
#include "util/delete.h"
#include "util/directory.h"
#include "util/fts.h"
#include "util/logging.h"
#include "util/path.h"
#include "util/selinux.h"
#include "util/string.h"

#include "apk.h"
#include "roms.h"

#define APP_DATA_SELINUX_CONTEXT        "u:object_r:app_data_file:s0"

#define APP_SHARING_APP_DIR             "/data/multiboot/_appsharing/app"
#define APP_SHARING_APP_ASEC_DIR        "/data/multiboot/_appsharing/app-asec"
#define APP_SHARING_DATA_DIR            "/data/multiboot/_appsharing/data"

#define USER_APP_DIR                    "/data/app"
#define USER_APP_ASEC_DIR               "/data/app-asec"
#define USER_DATA_DIR                   "/data/data"

static std::string _as_app_dir;
static std::string _as_app_asec_dir;
static std::string _as_data_dir;
static std::string _user_app_dir;
static std::string _user_app_asec_dir;
static std::string _user_data_dir;

namespace mb
{

/*!
 * Recursively chmod directories to 755 and files to 0644 and chown everything
 * system:system.
 */
class FixPermissions : public util::FTSWrapper {
public:
    FixPermissions(std::string path)
        : FTSWrapper(path, FTS_GroupSpecialFiles)
    {
    }

    virtual int on_changed_path() override
    {
        util::chown(_curr->fts_accpath, "system", "system", 0);
        return Action::FTS_OK;
    }

    virtual int on_reached_file() override
    {
        chmod(_curr->fts_accpath, 0644);
        return Action::FTS_OK;
    }

    virtual int on_reached_directory_pre() override
    {
        chmod(_curr->fts_accpath, 0755);
        return Action::FTS_OK;
    }
};

void AppSyncManager::detect_directories()
{
    _as_app_dir = get_raw_path(APP_SHARING_APP_DIR);
    _as_app_asec_dir = get_raw_path(APP_SHARING_APP_ASEC_DIR);
    _as_data_dir = get_raw_path(APP_SHARING_DATA_DIR);
    _user_app_dir = USER_APP_DIR;
    _user_app_asec_dir = USER_APP_ASEC_DIR;
    _user_data_dir = USER_DATA_DIR;

    LOGD("App sharing app directory:      {}", _as_app_dir);
    LOGD("App sharing app-asec directory: {}", _as_app_asec_dir);
    LOGD("App sharing app data directory: {}", _as_data_dir);
    LOGD("User app directory:             {}", _user_app_dir);
    LOGD("User app-asec directory:        {}", _user_app_asec_dir);
    LOGD("User app data directory:        {}", _user_data_dir);
}

/*!
 * \brief Get shared APK path for a package
 */
std::string AppSyncManager::get_shared_apk_path(const std::string &pkg)
{
    std::string path(_as_app_dir);
    path += "/";
    path += pkg;
    path += "/";
    path += "base.apk";
    return path;
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

/*!
 * \brief Copy the apk from USER_APP_DIR to APP_SHARING_APP_DIR
 *
 * If the shared apk already exists, the user apk will only be copied to the
 * shared apk if the version is newer.
 *
 * \note Make sure to set proper permissions on the shared apk directory and its
 *       contents after calling this function as some files may be owned by the
 *       uid mbtool is running as.
 *
 * \param pkg Package name
 *
 * \return True if shared apk is newer than the user apk or if the user apk was
 *         successfully copied to the shared apk. Otherwise, returns false.
 */
bool AppSyncManager::copy_apk_user_to_shared(const std::string &pkg)
{
    std::string shared_apk = get_shared_apk_path(pkg);
    std::string user_apk = find_apk(_user_app_dir, pkg);
    if (user_apk.empty()) {
        LOGW("{}: Failed to find apk for package {}", _user_app_dir, pkg);
        return false;
    }

    struct stat sb;
    if (stat(shared_apk.c_str(), &sb) == 0) {
        // Shared apk already exists, check version

        if (!S_ISREG(sb.st_mode)) {
            LOGW("{}: Shared apk path is not a regular file", shared_apk);
            return false;
        }

        ApkFile af_user;
        if (!af_user.open(user_apk)) {
            LOGW("{}: Failed to open or parse apk", user_apk);
            return false;
        }

        ApkFile af_shared;
        if (!af_shared.open(shared_apk)) {
            LOGW("{}: Failed to open or parse apk", shared_apk);
            return false;
        }

        if (af_user.package != af_shared.package) {
            LOGW("Conflicting package names between {} and {}",
                 user_apk, shared_apk);
            return false;
        }

        // User apk is the same version or older than the shared apk
        if (af_user.version_code <= af_shared.version_code) {
            LOGD("Not copying user apk to shared apk for package {}", pkg);
            LOGD("User version ({}) <= shared version ({})",
                 af_user.version_code, af_shared.version_code);
            return true;
        }
    } else if (errno != ENOENT) {
        // Continue only if the shared apk is missing
        LOGW("{}: stat() failed with errno != ENOENT: {}",
             shared_apk, strerror(errno));
        return false;
    }

    LOGD("Copying user apk to shared apk for package {}", pkg);
    LOGD("- User apk:   {}", user_apk);
    LOGD("- Shared apk: {}", shared_apk);

    if (!util::mkdir_parent(shared_apk, 0755)) {
        LOGW("Failed to create parent directories for {}", shared_apk);
        return false;
    }

    // Make sure we preserve the inode when copying, so all of the hard links to
    // the shared apk are preserved
    if (!util::copy_contents(user_apk, shared_apk)) {
        LOGW("Failed to copy contents from {} to {}", user_apk, shared_apk);
        return false;
    }

    return true;
}

bool AppSyncManager::link_apk_shared_to_user(const std::shared_ptr<Package> &pkg)
{
    std::string shared_apk = get_shared_apk_path(pkg->name);
    std::string user_apk = pkg->code_path;

    if (!util::ends_with(pkg->code_path, ".apk")) {
        user_apk += "/base.apk";
    }

    struct stat sb1;
    struct stat sb2;

    if (stat(shared_apk.c_str(), &sb1) < 0) {
        LOGW("{}: Failed to stat: {}", shared_apk, strerror(errno));
        return false;
    }

    if (stat(user_apk.c_str(), &sb2) == 0) {
        if (sb1.st_dev == sb2.st_dev && sb1.st_ino == sb2.st_ino) {
            // File is already linked correctly
            return true;
        }
        if (!S_ISREG(sb2.st_mode)) {
            LOGW("{}: apk is not a regular file", user_apk);
            return false;
        }
    }

    if (unlink(user_apk.c_str()) < 0 && errno != ENOENT) {
        LOGW("{}: Failed to unlink: {}", user_apk, strerror(errno));
        return false;
    }

    if (link(shared_apk.c_str(), user_apk.c_str()) < 0) {
        LOGW("Failed to hard link {} to {}: {}",
             shared_apk, user_apk, strerror(errno));
        return false;
    }

    return true;
}

bool AppSyncManager::wipe_shared_libraries(const std::shared_ptr<Package> &pkg)
{
    if (!util::delete_recursive(pkg->native_library_path)) {
        LOGW("{}: Failed to remove: {}",
             pkg->native_library_path, strerror(errno));
        return false;
    }

    return true;
}

bool AppSyncManager::initialize_directories()
{
    if (!util::mkdir_recursive(_as_app_dir, 0751) && errno != EEXIST) {
        LOGE("{}: Failed to create directory: {}", _as_app_dir,
             strerror(errno));
        return false;
    }
    if (!util::mkdir_recursive(_as_app_asec_dir, 0751) && errno != EEXIST) {
        LOGW("{}: Failed to create directory: {}", _as_app_asec_dir,
             strerror(errno));
        return false;
    }
    if (!util::mkdir_recursive(_as_data_dir, 0751) && errno != EEXIST) {
        LOGW("{}: Failed to create directory: {}", _as_data_dir,
             strerror(errno));
        return false;
    }

    return true;
}

bool AppSyncManager::create_shared_data_directory(const std::string &pkg, uid_t uid)
{
    std::string data_path = get_shared_data_path(pkg);

    if (!util::mkdir_recursive(data_path, 0751)) {
        LOGW("{}: Failed to create directory: {}", data_path, strerror(errno));
        return false;
    }

    // Ensure that the shared data directory permissions are correct
    if (chmod(data_path.c_str(), 0751) < 0) {
        LOGW("{}: Failed to chmod: {}", data_path, strerror(errno));
        return false;
    }

    if (!util::chown(data_path, uid, uid, util::MB_CHOWN_RECURSIVE)) {
        LOGW("{}: Failed to chown: {}", data_path, strerror(errno));
        return false;
    }

    return true;
}

bool AppSyncManager::fix_shared_apk_permissions()
{
    return FixPermissions(_as_app_dir).run();
}

bool AppSyncManager::fix_shared_data_permissions()
{
    if (!util::selinux_lset_context_recursive(
            _as_data_dir, APP_DATA_SELINUX_CONTEXT)) {
        LOGW("{}: Failed to set context recursively to {}: {}",
             _as_data_dir, APP_DATA_SELINUX_CONTEXT, strerror(errno));
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

    if (!util::mkdir_recursive(target, 0755)) {
        LOGW("{}: Failed to create directory: {}", target, strerror(errno));
        return false;
    }
    if (!util::chown(target, uid, uid, util::MB_CHOWN_RECURSIVE)) {
        LOGW("{}: Failed to chown: {}", target, strerror(errno));
        return false;
    }

    LOGV("Bind mounting data directory:");
    LOGV("- Source: {}", data_path);
    LOGV("- Target: {}", target);

    if (!unmount_shared_directory(pkg)) {
        return false;
    } else if (mount(data_path.c_str(), target.c_str(), "", MS_BIND, "") < 0) {
        LOGW("Failed to bind mount: {}", strerror(errno));
        return false;
    }

    return true;
}

bool AppSyncManager::unmount_shared_directory(const std::string &pkg)
{
    std::string target(_user_data_dir);
    target += "/";
    target += pkg;

    if (umount(target.c_str()) < 0 && errno != EINVAL) {
        LOGW("{}: Failed to unmount: {}", target, strerror(errno));
        return false;
    }

    return true;
}

}
