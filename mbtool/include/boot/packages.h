/*
 * Copyright (C) 2014-2018  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbcommon/flags.h"


namespace mb
{

class Package
{
public:
    // From frameworks/base/core/java/android/content/pm/ApplicationInfo.java
    // See https://android.googlesource.com/platform/frameworks/base/+/master/core/java/android/content/pm/ApplicationInfo.java
    enum class Flag : uint64_t
    {
        SYSTEM                    = 1ULL << 0,
        DEBUGGABLE                = 1ULL << 1,
        HAS_CODE                  = 1ULL << 2,
        PERSISTENT                = 1ULL << 3,
        FACTORY_TEST              = 1ULL << 4,
        ALLOW_TASK_REPARENTING    = 1ULL << 5,
        ALLOW_CLEAR_USER_DATA     = 1ULL << 6,
        UPDATED_SYSTEM_APP        = 1ULL << 7,
        TEST_ONLY                 = 1ULL << 8,
        SUPPORTS_SMALL_SCREENS    = 1ULL << 9,
        SUPPORTS_NORMAL_SCREENS   = 1ULL << 10,
        SUPPORTS_LARGE_SCREENS    = 1ULL << 11,
        RESIZEABLE_FOR_SCREENS    = 1ULL << 12,
        SUPPORTS_SCREEN_DENSITIES = 1ULL << 13,
        VM_SAFE_MODE              = 1ULL << 14,
        ALLOW_BACKUP              = 1ULL << 15,
        KILL_AFTER_RESTORE        = 1ULL << 16,
        RESTORE_ANY_VERSION       = 1ULL << 17,
        EXTERNAL_STORAGE          = 1ULL << 18,
        SUPPORTS_XLARGE_SCREENS   = 1ULL << 19,
        LARGE_HEAP                = 1ULL << 20,
        STOPPED                   = 1ULL << 21,
        SUPPORTS_RTL              = 1ULL << 22,
        INSTALLED                 = 1ULL << 23,
        IS_DATA_ONLY              = 1ULL << 24,
        IS_GAME                   = 1ULL << 25,
        FULL_BACKUP_ONLY          = 1ULL << 26,
        HIDDEN                    = 1ULL << 27,
        CANT_SAVE_STATE           = 1ULL << 28,
        FORWARD_LOCK              = 1ULL << 29,
        PRIVILEGED                = 1ULL << 30,
        MULTIARCH                 = 1ULL << 31,
    };
    MB_DECLARE_FLAGS(Flags, Flag)

    enum class PublicFlag : uint64_t
    {
        SYSTEM                    = 1ULL << 0,
        DEBUGGABLE                = 1ULL << 1,
        HAS_CODE                  = 1ULL << 2,
        PERSISTENT                = 1ULL << 3,
        FACTORY_TEST              = 1ULL << 4,
        ALLOW_TASK_REPARENTING    = 1ULL << 5,
        ALLOW_CLEAR_USER_DATA     = 1ULL << 6,
        UPDATED_SYSTEM_APP        = 1ULL << 7,
        TEST_ONLY                 = 1ULL << 8,
        SUPPORTS_SMALL_SCREENS    = 1ULL << 9,
        SUPPORTS_NORMAL_SCREENS   = 1ULL << 10,
        SUPPORTS_LARGE_SCREENS    = 1ULL << 11,
        RESIZEABLE_FOR_SCREENS    = 1ULL << 12,
        SUPPORTS_SCREEN_DENSITIES = 1ULL << 13,
        VM_SAFE_MODE              = 1ULL << 14,
        ALLOW_BACKUP              = 1ULL << 15,
        KILL_AFTER_RESTORE        = 1ULL << 16,
        RESTORE_ANY_VERSION       = 1ULL << 17,
        EXTERNAL_STORAGE          = 1ULL << 18,
        SUPPORTS_XLARGE_SCREENS   = 1ULL << 19,
        LARGE_HEAP                = 1ULL << 20,
        STOPPED                   = 1ULL << 21,
        SUPPORTS_RTL              = 1ULL << 22,
        INSTALLED                 = 1ULL << 23,
        IS_DATA_ONLY              = 1ULL << 24,
        IS_GAME                   = 1ULL << 25,
        FULL_BACKUP_ONLY          = 1ULL << 26,
        USES_CLEARTEXT_TRAFFIC    = 1ULL << 27,
        EXTRACT_NATIVE_LIBS       = 1ULL << 28,
        HARDWARE_ACCELERATED      = 1ULL << 29,
        SUSPENDED                 = 1ULL << 30,
        MULTIARCH                 = 1ULL << 31,
    };
    MB_DECLARE_FLAGS(PublicFlags, PublicFlag)

    enum class PrivateFlag : uint64_t
    {
        HIDDEN                                            = 1ULL << 0,
        CANT_SAVE_STATE                                   = 1ULL << 1,
        FORWARD_LOCK                                      = 1ULL << 2,
        PRIVILEGED                                        = 1ULL << 3,
        HAS_DOMAIN_URLS                                   = 1ULL << 4,
        DEFAULT_TO_DEVICE_PROTECTED_STORAGE               = 1ULL << 5,
        DIRECT_BOOT_AWARE                                 = 1ULL << 6,
        INSTANT                                           = 1ULL << 7,
        PARTIALLY_DIRECT_BOOT_AWARE                       = 1ULL << 8,
        REQUIRED_FOR_SYSTEM_USER                          = 1ULL << 9,
        ACTIVITIES_RESIZE_MODE_RESIZEABLE                 = 1ULL << 10,
        ACTIVITIES_RESIZE_MODE_UNRESIZEABLE               = 1ULL << 11,
        ACTIVITIES_RESIZE_MODE_RESIZEABLE_VIA_SDK_VERSION = 1ULL << 12,
        BACKUP_IN_FOREGROUND                              = 1ULL << 13,
        STATIC_SHARED_LIBRARY                             = 1ULL << 14,
        ISOLATED_SPLIT_LOADING                            = 1ULL << 15,
        VIRTUAL_PRELOAD                                   = 1ULL << 16,
    };
    MB_DECLARE_FLAGS(PrivateFlags, PrivateFlag)

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

MB_DECLARE_OPERATORS_FOR_FLAGS(Package::Flags)
MB_DECLARE_OPERATORS_FOR_FLAGS(Package::PublicFlags)
MB_DECLARE_OPERATORS_FOR_FLAGS(Package::PrivateFlags)

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
