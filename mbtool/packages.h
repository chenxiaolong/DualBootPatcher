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

#pragma once

#include <vector>


class Package {
public:
    // From frameworks/base/core/java/android/content/pm/ApplicationInfo.java
    // See https://android.googlesource.com/platform/frameworks/base/+/master/core/java/android/content/pm/ApplicationInfo.java
    enum class Flags : int {
        FLAG_SYSTEM                    = 1 << 0,
        FLAG_DEBUGGABLE                = 1 << 1,
        FLAG_HAS_CODE                  = 1 << 2,
        FLAG_PERSISTENT                = 1 << 3,
        FLAG_FACTORY_TEST              = 1 << 4,
        FLAG_ALLOW_TASK_REPARENTING    = 1 << 5,
        FLAG_ALLOW_CLEAR_USER_DATA     = 1 << 6,
        FLAG_UPDATED_SYSTEM_APP        = 1 << 7,
        FLAG_TEST_ONLY                 = 1 << 8,
        FLAG_SUPPORTS_SMALL_SCREENS    = 1 << 9,
        FLAG_SUPPORTS_NORMAL_SCREENS   = 1 << 10,
        FLAG_SUPPORTS_LARGE_SCREENS    = 1 << 11,
        FLAG_RESIZEABLE_FOR_SCREENS    = 1 << 12,
        FLAG_SUPPORTS_SCREEN_DENSITIES = 1 << 13,
        FLAG_VM_SAFE_MODE              = 1 << 14,
        FLAG_ALLOW_BACKUP              = 1 << 15,
        FLAG_KILL_AFTER_RESTORE        = 1 << 16,
        FLAG_RESTORE_ANY_VERSION       = 1 << 17,
        FLAG_EXTERNAL_STORAGE          = 1 << 18,
        FLAG_SUPPORTS_XLARGE_SCREENS   = 1 << 19,
        FLAG_LARGE_HEAP                = 1 << 20,
        FLAG_STOPPED                   = 1 << 21,
        FLAG_SUPPORTS_RTL              = 1 << 22,
        FLAG_INSTALLED                 = 1 << 23,
        FLAG_IS_DATA_ONLY              = 1 << 24,
        FLAG_IS_GAME                   = 1 << 25,
        FLAG_FULL_BACKUP_ONLY          = 1 << 26,
        FLAG_HIDDEN                    = 1 << 27,
        FLAG_CANT_SAVE_STATE           = 1 << 28,
        FLAG_FORWARD_LOCK              = 1 << 29,
        FLAG_PRIVILEGED                = 1 << 30,
        FLAG_MULTIARCH                 = 1 << 31
    };

    char *name;                                 // PackageSetting.name
    char *real_name;                            // PackageSetting.realName
    char *code_path;                            // PackageSetting.codePathString
    char *resource_path;                        // PackageSetting.resourcePathString
    char *native_library_path;                  // PackageSetting.legacyNativeLibraryPathString
    char *primary_cpu_abi;                      // PackageSetting.primaryCpuAbiString
    char *secondary_cpu_abi;                    // PackageSetting.secondaryCpuAbiString
    char *cpu_abi_override;                     // PackageSetting.cpuAbiOverride
    int pkg_flags;                              // PackageSetting.pkgFlags
    // Timestamps are in milliseconds epoch/unix time
    unsigned long long timestamp;               // PackageSetting.timeStamp
    unsigned long long first_install_time;      // PackageSetting.firstInstallTime
    unsigned long long last_update_time;        // PackageSetting.lastUpdateTime
    int version;                                // PackageSetting.versionCode
    int is_shared_user;                         // PackageSetting.sharedUser != null
    int user_id;                                // PackageSetting.appId
    int shared_user_id;                         // PackageSetting.appId
    char *uid_error;                            // (not in PackageSetting)
    char *install_status;                       // (not in PackageSetting)
    char *installer;                            // PackageSetting.installerPackageName

    // Functions
    Package();
    ~Package();
    bool load_xml(const char *path);
};

int mb_packages_load_xml(std::vector<Package *> *pkgs, const char *path);
void mb_packages_free(std::vector<Package *> *pkgs);