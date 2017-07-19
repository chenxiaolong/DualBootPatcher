/*
 * Copyright (C) 2014-2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include <string>

#define INTERNAL_STORAGE                "/data/media/0"
#define MULTIBOOT_DIR                   INTERNAL_STORAGE "/MultiBoot"
#define MULTIBOOT_BACKUP_DIR            MULTIBOOT_DIR "/backups"
#define MULTIBOOT_LOG_INSTALLER         INTERNAL_STORAGE "/MultiBoot.log"
#define MULTIBOOT_LOG_APPSYNC           MULTIBOOT_DIR "/appsync.log"
#define MULTIBOOT_LOG_DAEMON            MULTIBOOT_DIR "/daemon.log"

#define ABOOT_PARTITION                 "/dev/block/platform/msm_sdcc.1/by-name/aboot"

#define UNIVERSAL_BY_NAME_DIR           "/dev/block/by-name"

#define PACKAGES_XML                    "/data/system/packages.xml"

#define DEFAULT_PROP_PATH               "/default.prop"

#define DEVICE_JSON_PATH                "/device.json"

#define FILE_CONTEXTS_BIN               "/file_contexts.bin"
#define FILE_CONTEXTS                   "/file_contexts"

#define PROP_BLOCK_DEV_BASE_DIRS        "ro.patcher.blockdevs.base"
#define PROP_BLOCK_DEV_SYSTEM_PATHS     "ro.patcher.blockdevs.system"
#define PROP_BLOCK_DEV_CACHE_PATHS      "ro.patcher.blockdevs.cache"
#define PROP_BLOCK_DEV_DATA_PATHS       "ro.patcher.blockdevs.data"
#define PROP_BLOCK_DEV_BOOT_PATHS       "ro.patcher.blockdevs.boot"
#define PROP_BLOCK_DEV_RECOVERY_PATHS   "ro.patcher.blockdevs.recovery"
#define PROP_BLOCK_DEV_EXTRA_PATHS      "ro.patcher.blockdevs.extra"
#define PROP_USE_FUSE_EXFAT             "ro.patcher.use_fuse_exfat"

#define PROP_MULTIBOOT_VERSION          "ro.multiboot.version"
#define PROP_MULTIBOOT_ROM_ID           "ro.multiboot.romid"

// Boot UI
#define BOOT_UI_SKIP_PATH               "/raw/cache/multiboot/bootui/skip"
#define BOOT_UI_ZIP_PATH                "/raw/cache/multiboot/bootui.zip"
#define BOOT_UI_PATH                    "/mbbootui"
#define BOOT_UI_EXEC_PATH               BOOT_UI_PATH "/exec"

// Installer
#define CHROOT_SYSTEM_BIND_MOUNT        "/mb/bind.system"
#define CHROOT_CACHE_BIND_MOUNT         "/mb/bind.cache"
#define CHROOT_DATA_BIND_MOUNT          "/mb/bind.data"
#define CHROOT_SYSTEM_LOOP_DEV          "/mb/loop.system"
#define CHROOT_CACHE_LOOP_DEV           "/mb/loop.cache"
#define CHROOT_DATA_LOOP_DEV            "/mb/loop.data"

// SELinux context for mbtool utils
#define MB_EXEC_CONTEXT                 "u:r:mb_exec:s0"

namespace mb
{

bool copy_system(const std::string &source, const std::string &target);

bool fix_multiboot_permissions();

bool switch_context(const std::string &context);

}
