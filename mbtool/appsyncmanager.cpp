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

#include <algorithm>

#include <cerrno>

#include <sys/mount.h>
#include <sys/stat.h>
#include <unistd.h>

#include "util/chown.h"
#include "util/command.h"
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

    LOGD("App sharing app directory:      %s", _as_app_dir.c_str());
    LOGD("App sharing app-asec directory: %s", _as_app_asec_dir.c_str());
    LOGD("App sharing app data directory: %s", _as_data_dir.c_str());
    LOGD("User app directory:             %s", _user_app_dir.c_str());
    LOGD("User app-asec directory:        %s", _user_app_asec_dir.c_str());
    LOGD("User app data directory:        %s", _user_data_dir.c_str());
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
        LOGW("[%s] %s: Failed to find apk",
             pkg.c_str(), _user_app_dir.c_str());
        return false;
    }

    struct stat sb;
    if (stat(shared_apk.c_str(), &sb) == 0) {
        // Shared apk already exists, check version

        if (!S_ISREG(sb.st_mode)) {
            LOGW("[%s] %s: Shared apk path is not a regular file",
                 pkg.c_str(), shared_apk.c_str());
            return false;
        }

        ApkFile af_user;
        if (!af_user.open(user_apk)) {
            LOGW("[%s] %s: Failed to open or parse apk",
                 pkg.c_str(), user_apk.c_str());
            return false;
        }

        ApkFile af_shared;
        if (!af_shared.open(shared_apk)) {
            LOGW("[%s] %s: Failed to open or parse apk",
                 pkg.c_str(), shared_apk.c_str());
            return false;
        }

        if (af_user.package != af_shared.package) {
            LOGW("[%s] Conflicting package names between %s and %s",
                 pkg.c_str(), user_apk.c_str(), shared_apk.c_str());
            return false;
        }

        // User apk is the same version or older than the shared apk
        if (af_user.version_code <= af_shared.version_code) {
            LOGI("[%s] Skipped user -> shared copy because "
                 "user version (%u) <= shared version (%u)",
                 pkg.c_str(), af_user.version_code, af_shared.version_code);
            return true;
        }
    } else if (errno != ENOENT) {
        // Continue only if the shared apk is missing
        LOGW("[%s] %s: stat() failed with errno != ENOENT: %s",
             pkg.c_str(), shared_apk.c_str(), strerror(errno));
        return false;
    }

    LOGI("[%s] Copying user apk to shared apk", pkg.c_str());
    LOGI("[%s] - User apk:   %s", pkg.c_str(), user_apk.c_str());
    LOGI("[%s] - Shared apk: %s", pkg.c_str(), shared_apk.c_str());

    if (!util::mkdir_parent(shared_apk, 0755)) {
        LOGW("[%s] Failed to create parent directories for %s",
             pkg.c_str(), shared_apk.c_str());
        return false;
    }

    // Make sure we preserve the inode when copying, so all of the hard links to
    // the shared apk are preserved
    if (!util::copy_contents(user_apk, shared_apk)) {
        LOGW("[%s] Failed to copy contents from %s to %s",
             pkg.c_str(), user_apk.c_str(), shared_apk.c_str());
        return false;
    }

    return true;
}

/*!
 * For each installed ROM, if the apk is shared, unlink the apk and hard link
 * the shared apk to it.
 */
bool AppSyncManager::sync_apk_shared_to_user(const std::string &pkgname,
                                             const std::vector<RomConfigAndPackages> &cfg_pkgs_list)
{
    LOGD("[%s] Syncing to all ROMs where it is shared", pkgname.c_str());

    std::vector<std::string> roms_all;
    std::vector<std::string> roms_linked;
    std::vector<std::string> roms_not_linked;

    std::string shared_apk = get_shared_apk_path(pkgname);

    struct stat sb_shared;
    if (stat(shared_apk.c_str(), &sb_shared) < 0) {
        LOGE("[%s] %s: Failed to stat: %s",
             pkgname.c_str(), shared_apk.c_str(), strerror(errno));
        return false;
    }

    for (const RomConfigAndPackages &cfg_pkgs : cfg_pkgs_list) {
        const std::shared_ptr<Rom> &rom = cfg_pkgs.rom;
        const RomConfig &config = cfg_pkgs.config;
        const Packages &packages = cfg_pkgs.packages;

        // Ensure the package is installed in this ROM
        auto pkg = packages.find_by_pkg(pkgname);
        if (!pkg) {
            continue;
        }

        roms_all.push_back(rom->id);

        // Ensure the user wants the app to be shared in this ROM
        auto it = std::find_if(config.shared_pkgs.begin(),
                               config.shared_pkgs.end(),
                               [&](const SharedPackage &shared_pkg){
            return shared_pkg.pkg_id == pkgname;
        });
        if (it == config.shared_pkgs.end()) {
            continue;
        }

        // Ensure that the user apk resides in /data/app
        if (!util::starts_with(pkg->code_path, _user_app_dir)) {
            LOGW("[%s] %s: Does not reside in /data [ROM: %s]",
                 pkgname.c_str(), pkg->code_path.c_str(), rom->id.c_str());
            continue;
        }

        // We need to get the non-bind mounted path since pre-Android 5.0 libc
        // has a bug linking across bind mounts, even if st_dev is the same.
        std::string user_apk;
        user_apk += rom->data_path;
        user_apk += pkg->code_path.substr(5); // For "/data"
        if (!util::ends_with(pkg->code_path, ".apk")) {
            // Android >= 5.0
            user_apk += "/base.apk";
        }

        struct stat sb_user;

        // Try to stat the user apk
        if (stat(user_apk.c_str(), &sb_user) == 0) {
            if (sb_shared.st_dev == sb_user.st_dev
                    && sb_shared.st_ino == sb_user.st_ino) {
                // User apk is already linked to the shared apk. Move on to the
                // next ROM
                roms_linked.push_back(rom->id);
                continue;
            }
            if (!S_ISREG(sb_user.st_mode)) {
                // User apk is not a regular file. Skip and move on to the next
                // ROM
                LOGW("[%s] %s: apk is not a regular file [ROM: %s]",
                     pkgname.c_str(), user_apk.c_str(), rom->id.c_str());
                continue;
            }
        }

        roms_not_linked.push_back(rom->id);

        LOGI("[%s] Linking shared apk to user apk [ROM: %s]",
             pkgname.c_str(), rom->id.c_str());

        // Remove old user apk
        if (unlink(user_apk.c_str()) < 0 && errno != ENOENT) {
            LOGW("[%s] %s: Failed to unlink: %s",
                 pkgname.c_str(), user_apk.c_str(), strerror(errno));
            return false;
        }

        // Hard link shared apk to user apk
        if (link(shared_apk.c_str(), user_apk.c_str()) < 0) {
            LOGW("[%s] Failed to hard link %s to %s: %s", pkgname.c_str(),
                 shared_apk.c_str(), user_apk.c_str(), strerror(errno));
            return false;
        }
    }

    if (!roms_all.empty()) {
        LOGI("[%s] Shared in:      %s", pkgname.c_str(),
             util::join(roms_all, ", ").c_str());
        LOGI("[%s] Already linked: %s", pkgname.c_str(),
             roms_linked.empty() ? "(none)" : util::join(roms_linked, ", ").c_str());
        LOGI("[%s] Newly linked:   %s", pkgname.c_str(),
             roms_not_linked.empty() ? "(none)" : util::join(roms_not_linked, ", ").c_str());
    }

    return true;
}

bool AppSyncManager::wipe_shared_libraries(const std::shared_ptr<Package> &pkg)
{
    if (!util::delete_recursive(pkg->native_library_path)) {
        LOGW("[%s] %s: Failed to remove: %s", pkg->name.c_str(),
             pkg->native_library_path.c_str(), strerror(errno));
        return false;
    }

    LOGV("[%s] Removed shared libraries at %s",
         pkg->name.c_str(), pkg->native_library_path.c_str());

    return true;
}

bool AppSyncManager::initialize_directories()
{
    if (!util::mkdir_recursive(_as_app_dir, 0751) && errno != EEXIST) {
        LOGE("%s: Failed to create directory: %s", _as_app_dir.c_str(),
             strerror(errno));
        return false;
    }
    if (!util::mkdir_recursive(_as_app_asec_dir, 0751) && errno != EEXIST) {
        LOGW("%s: Failed to create directory: %s", _as_app_asec_dir.c_str(),
             strerror(errno));
        return false;
    }
    if (!util::mkdir_recursive(_as_data_dir, 0751) && errno != EEXIST) {
        LOGW("%s: Failed to create directory: %s", _as_data_dir.c_str(),
             strerror(errno));
        return false;
    }

    return true;
}

bool AppSyncManager::create_shared_data_directory(const std::string &pkg, uid_t uid)
{
    std::string data_path = get_shared_data_path(pkg);

    if (!util::mkdir_recursive(data_path, 0751)) {
        LOGW("[%s] %s: Failed to create directory: %s",
             pkg.c_str(), data_path.c_str(), strerror(errno));
        return false;
    }

    // Ensure that the shared data directory permissions are correct
    if (chmod(data_path.c_str(), 0751) < 0) {
        LOGW("[%s] %s: Failed to chmod: %s",
             pkg.c_str(), data_path.c_str(), strerror(errno));
        return false;
    }

    if (!util::chown(data_path, uid, uid, util::CHOWN_RECURSIVE)) {
        LOGW("[%s] %s: Failed to chown: %s",
             pkg.c_str(), data_path.c_str(), strerror(errno));
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
        LOGW("%s: Failed to set context recursively to %s: %s",
             _as_data_dir.c_str(), APP_DATA_SELINUX_CONTEXT, strerror(errno));
        return false;
    }

    return true;
}

bool AppSyncManager::fix_user_apk_context()
{
    return util::run_command({ "restorecon", "-R", "-F", _user_app_dir }) == 0;
}

bool AppSyncManager::mount_shared_directory(const std::string &pkg, uid_t uid)
{
    std::string data_path = get_shared_data_path(pkg);
    std::string target(_user_data_dir);
    target += "/";
    target += pkg;

    if (!util::mkdir_recursive(target, 0755)) {
        LOGW("[%s] %s: Failed to create directory: %s",
             pkg.c_str(), target.c_str(), strerror(errno));
        return false;
    }
    if (!util::chown(target, uid, uid, util::CHOWN_RECURSIVE)) {
        LOGW("[%s] %s: Failed to chown: %s",
             pkg.c_str(), target.c_str(), strerror(errno));
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
