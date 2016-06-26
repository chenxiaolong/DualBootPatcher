/*
 * Copyright (C) 2014-2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include <string>
#include <vector>

#define INTERNAL_STORAGE                "/data/media/0"
#define MULTIBOOT_DIR                   INTERNAL_STORAGE "/MultiBoot"
#define MULTIBOOT_BACKUP_DIR            MULTIBOOT_DIR "/backups"
#define MULTIBOOT_LOG_INSTALLER         INTERNAL_STORAGE "/MultiBoot.log"
#define MULTIBOOT_LOG_APPSYNC           MULTIBOOT_DIR "/appsync.log"
#define MULTIBOOT_LOG_DAEMON            MULTIBOOT_DIR "/daemon.log"
#define MULTIBOOT_LOG_KERNEL            MULTIBOOT_DIR "/kernel.log"

#define ABOOT_PARTITION                 "/dev/block/platform/msm_sdcc.1/by-name/aboot"

#define UNIVERSAL_BY_NAME_DIR           "/dev/block/by-name"

#define PACKAGES_XML                    "/data/system/packages.xml"

#define DEFAULT_PROP_PATH               "/default.prop"

#define PROP_BLOCK_DEV_BASE_DIRS        "ro.patcher.blockdevs.base"
#define PROP_BLOCK_DEV_SYSTEM_PATHS     "ro.patcher.blockdevs.system"
#define PROP_BLOCK_DEV_CACHE_PATHS      "ro.patcher.blockdevs.cache"
#define PROP_BLOCK_DEV_DATA_PATHS       "ro.patcher.blockdevs.data"
#define PROP_BLOCK_DEV_RECOVERY_PATHS   "ro.patcher.blockdevs.recovery"
#define PROP_BLOCK_DEV_EXTRA_PATHS      "ro.patcher.blockdevs.extra"

// Installer
#define CHROOT_SYSTEM_LOOP_DEV          "/mb/loop.system"
#define CHROOT_CACHE_LOOP_DEV           "/mb/loop.cache"
#define CHROOT_DATA_LOOP_DEV            "/mb/loop.data"

namespace mb
{

bool copy_system(const std::string &source, const std::string &target);

bool fix_multiboot_permissions(void);

std::vector<std::string> decode_list(const std::string &encoded);

}
