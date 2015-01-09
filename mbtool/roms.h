/*
 * Copyright (C) 2014  Xiao-Long Chen <chenxiaolong@cxl.epac.to>
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

#include <stddef.h>

// From frameworks/base/core/java/android/content/pm/ApplicationInfo.java
// See https://android.googlesource.com/platform/frameworks/base/+/master/core/java/android/content/pm/ApplicationInfo.java
#define FLAG_SYSTEM                     1 << 0
#define FLAG_DEBUGGABLE                 1 << 1
#define FLAG_HAS_CODE                   1 << 2
#define FLAG_PERSISTENT                 1 << 3
#define FLAG_FACTORY_TEST               1 << 4
#define FLAG_ALLOW_TASK_REPARENTING     1 << 5
#define FLAG_ALLOW_CLEAR_USER_DATA      1 << 6
#define FLAG_UPDATED_SYSTEM_APP         1 << 7
#define FLAG_TEST_ONLY                  1 << 8
#define FLAG_SUPPORTS_SMALL_SCREENS     1 << 9
#define FLAG_SUPPORTS_NORMAL_SCREENS    1 << 10
#define FLAG_SUPPORTS_LARGE_SCREENS     1 << 11
#define FLAG_RESIZEABLE_FOR_SCREENS     1 << 12
#define FLAG_SUPPORTS_SCREEN_DENSITIES  1 << 13
#define FLAG_VM_SAFE_MODE               1 << 14
#define FLAG_ALLOW_BACKUP               1 << 15
#define FLAG_KILL_AFTER_RESTORE         1 << 16
#define FLAG_RESTORE_ANY_VERSION        1 << 17
#define FLAG_EXTERNAL_STORAGE           1 << 18
#define FLAG_SUPPORTS_XLARGE_SCREENS    1 << 19
#define FLAG_LARGE_HEAP                 1 << 20
#define FLAG_STOPPED                    1 << 21
#define FLAG_SUPPORTS_RTL               1 << 22
#define FLAG_INSTALLED                  1 << 23
#define FLAG_IS_DATA_ONLY               1 << 24
#define FLAG_IS_GAME                    1 << 25
#define FLAG_FULL_BACKUP_ONLY           1 << 26
#define FLAG_HIDDEN                     1 << 27
#define FLAG_CANT_SAVE_STATE            1 << 28
#define FLAG_FORWARD_LOCK               1 << 29
#define FLAG_PRIVILEGED                 1 << 30
#define FLAG_MULTIARCH                  1 << 31

struct package {
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
};

struct rom {
    char *id;
    char *kernel_id;
    char *name;
    char *description;
    int system_uses_image;
    char *system_path;
    char *cache_path;
    char *data_path;
    struct package *pkgs;
    size_t pkgs_len;
};

struct roms {
    struct rom *list;
    size_t len;
};

int mb_roms_init(struct roms *roms);
int mb_roms_cleanup(struct roms *roms);
int mb_roms_get_builtin(struct roms *roms);
struct rom * mb_find_rom_by_id(struct roms *roms, const char *id);
int mb_rom_load_packages(struct rom *rom);
int mb_rom_cleanup_packages(struct rom *rom);

int ROMTEST_main(int argc, char *argv[]);
int PKGTEST_main(int argc, char *argv[]);
