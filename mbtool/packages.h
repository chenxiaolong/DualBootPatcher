/*
 * Copyright (C) 2014-2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>


namespace mb
{

class Package
{
public:
    // From frameworks/base/core/java/android/content/pm/ApplicationInfo.java
    // See https://android.googlesource.com/platform/frameworks/base/+/master/core/java/android/content/pm/ApplicationInfo.java
    enum Flags : uint64_t {
        FLAG_SYSTEM                    = 1ULL << 0,
        FLAG_DEBUGGABLE                = 1ULL << 1,
        FLAG_HAS_CODE                  = 1ULL << 2,
        FLAG_PERSISTENT                = 1ULL << 3,
        FLAG_FACTORY_TEST              = 1ULL << 4,
        FLAG_ALLOW_TASK_REPARENTING    = 1ULL << 5,
        FLAG_ALLOW_CLEAR_USER_DATA     = 1ULL << 6,
        FLAG_UPDATED_SYSTEM_APP        = 1ULL << 7,
        FLAG_TEST_ONLY                 = 1ULL << 8,
        FLAG_SUPPORTS_SMALL_SCREENS    = 1ULL << 9,
        FLAG_SUPPORTS_NORMAL_SCREENS   = 1ULL << 10,
        FLAG_SUPPORTS_LARGE_SCREENS    = 1ULL << 11,
        FLAG_RESIZEABLE_FOR_SCREENS    = 1ULL << 12,
        FLAG_SUPPORTS_SCREEN_DENSITIES = 1ULL << 13,
        FLAG_VM_SAFE_MODE              = 1ULL << 14,
        FLAG_ALLOW_BACKUP              = 1ULL << 15,
        FLAG_KILL_AFTER_RESTORE        = 1ULL << 16,
        FLAG_RESTORE_ANY_VERSION       = 1ULL << 17,
        FLAG_EXTERNAL_STORAGE          = 1ULL << 18,
        FLAG_SUPPORTS_XLARGE_SCREENS   = 1ULL << 19,
        FLAG_LARGE_HEAP                = 1ULL << 20,
        FLAG_STOPPED                   = 1ULL << 21,
        FLAG_SUPPORTS_RTL              = 1ULL << 22,
        FLAG_INSTALLED                 = 1ULL << 23,
        FLAG_IS_DATA_ONLY              = 1ULL << 24,
        FLAG_IS_GAME                   = 1ULL << 25,
        FLAG_FULL_BACKUP_ONLY          = 1ULL << 26,
        FLAG_HIDDEN                    = 1ULL << 27,
        FLAG_CANT_SAVE_STATE           = 1ULL << 28,
        FLAG_FORWARD_LOCK              = 1ULL << 29,
        FLAG_PRIVILEGED                = 1ULL << 30,
        FLAG_MULTIARCH                 = 1ULL << 31
    };

    enum PublicFlags : uint64_t {
        PUBLIC_FLAG_SYSTEM                    = 1ULL << 0,
        PUBLIC_FLAG_DEBUGGABLE                = 1ULL << 1,
        PUBLIC_FLAG_HAS_CODE                  = 1ULL << 2,
        PUBLIC_FLAG_PERSISTENT                = 1ULL << 3,
        PUBLIC_FLAG_FACTORY_TEST              = 1ULL << 4,
        PUBLIC_FLAG_ALLOW_TASK_REPARENTING    = 1ULL << 5,
        PUBLIC_FLAG_ALLOW_CLEAR_USER_DATA     = 1ULL << 6,
        PUBLIC_FLAG_UPDATED_SYSTEM_APP        = 1ULL << 7,
        PUBLIC_FLAG_TEST_ONLY                 = 1ULL << 8,
        PUBLIC_FLAG_SUPPORTS_SMALL_SCREENS    = 1ULL << 9,
        PUBLIC_FLAG_SUPPORTS_NORMAL_SCREENS   = 1ULL << 10,
        PUBLIC_FLAG_SUPPORTS_LARGE_SCREENS    = 1ULL << 11,
        PUBLIC_FLAG_RESIZEABLE_FOR_SCREENS    = 1ULL << 12,
        PUBLIC_FLAG_SUPPORTS_SCREEN_DENSITIES = 1ULL << 13,
        PUBLIC_FLAG_VM_SAFE_MODE              = 1ULL << 14,
        PUBLIC_FLAG_ALLOW_BACKUP              = 1ULL << 15,
        PUBLIC_FLAG_KILL_AFTER_RESTORE        = 1ULL << 16,
        PUBLIC_FLAG_RESTORE_ANY_VERSION       = 1ULL << 17,
        PUBLIC_FLAG_EXTERNAL_STORAGE          = 1ULL << 18,
        PUBLIC_FLAG_SUPPORTS_XLARGE_SCREENS   = 1ULL << 19,
        PUBLIC_FLAG_LARGE_HEAP                = 1ULL << 20,
        PUBLIC_FLAG_STOPPED                   = 1ULL << 21,
        PUBLIC_FLAG_SUPPORTS_RTL              = 1ULL << 22,
        PUBLIC_FLAG_INSTALLED                 = 1ULL << 23,
        PUBLIC_FLAG_IS_DATA_ONLY              = 1ULL << 24,
        PUBLIC_FLAG_IS_GAME                   = 1ULL << 25,
        PUBLIC_FLAG_FULL_BACKUP_ONLY          = 1ULL << 26,
        PUBLIC_FLAG_USES_CLEARTEXT_TRAFFIC    = 1ULL << 27,
        PUBLIC_FLAG_EXTRACT_NATIVE_LIBS       = 1ULL << 28,
        PUBLIC_FLAG_HARDWARE_ACCELERATED      = 1ULL << 29,
        // No 1ULL << 30
        PUBLIC_FLAG_MULTIARCH                 = 1ULL << 31
    };

    enum PrivateFlags : uint64_t {
        PRIVATE_FLAG_HIDDEN          = 1ULL << 0,
        PRIVATE_FLAG_CANT_SAVE_STATE = 1ULL << 1,
        PRIVATE_FLAG_FORWARD_LOCK    = 1ULL << 2,
        PRIVATE_FLAG_PRIVILEGED      = 1ULL << 3,
        PRIVATE_FLAG_HAS_DOMAIN_URLS = 1ULL << 4
    };

    std::string name;                   // PackageSetting.name
    std::string real_name;              // PackageSetting.realName
    std::string code_path;              // PackageSetting.codePathString
    std::string resource_path;          // PackageSetting.resourcePathString
    std::string native_library_path;    // PackageSetting.legacyNativeLibraryPathString
    std::string primary_cpu_abi;        // PackageSetting.primaryCpuAbiString
    std::string secondary_cpu_abi;      // PackageSetting.secondaryCpuAbiString
    std::string cpu_abi_override;       // PackageSetting.cpuAbiOverride
    // Android <6.0
    Flags pkg_flags;                    // PackageSetting.pkgFlags
    // Android >=6.0
    PublicFlags pkg_public_flags;       // PackageSetting.pkgFlags
    PrivateFlags pkg_private_flags;     // PackageSetting.pkgPrivateFlags
    // Timestamps are in milliseconds epoch/unix time
    uint64_t timestamp;                 // PackageSetting.timeStamp
    uint64_t first_install_time;        // PackageSetting.firstInstallTime
    uint64_t last_update_time;          // PackageSetting.lastUpdateTime
    int version;                        // PackageSetting.versionCode
    int is_shared_user;                 // PackageSetting.sharedUser != null
    int user_id;                        // PackageSetting.appId
    int shared_user_id;                 // PackageSetting.appId
    std::string uid_error;              // (not in PackageSetting)
    std::string install_status;         // (not in PackageSetting)
    std::string installer;              // PackageSetting.installerPackageName

    std::vector<std::string> sig_indexes;

    // Functions
    Package();

    uid_t get_uid();

    void dump();
};

class Packages
{
public:
    std::vector<std::shared_ptr<Package>> pkgs;
    std::unordered_map<std::string, std::string> sigs;

    bool load_xml(const std::string &path);

    std::shared_ptr<Package> find_by_uid(uid_t uid) const;
    std::shared_ptr<Package> find_by_pkg(const std::string &pkg_id) const;
};

}
