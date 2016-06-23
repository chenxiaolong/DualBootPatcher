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

#include "roms.h"

namespace mb
{

enum MountFlags : int
{
    // Rewrite fstab file to remove mounted entries
    MOUNT_FLAG_REWRITE_FSTAB            = 0x1,
    // Prevent the use of generic fstab entries for fstab files that are missing
    // entries for /system, /cache, or /data
    MOUNT_FLAG_NO_GENERIC_ENTRIES       = 0x2,
    // Unmount mount points that were successfully mounted if a later entry in
    // the fstab file fails to mount (affects only the mount points mounted in
    // the current invocation of the function)
    MOUNT_FLAG_UNMOUNT_ON_FAILURE       = 0x4,

    // Skip /system
    MOUNT_FLAG_SKIP_SYSTEM              = 0x100,
    // Skip /cache
    MOUNT_FLAG_SKIP_CACHE               = 0x200,
    // Skip /data
    MOUNT_FLAG_SKIP_DATA                = 0x400,
    // Skip external SD
    MOUNT_FLAG_SKIP_EXTERNAL_SD         = 0x800,
};

bool mount_fstab(const char *path, const std::shared_ptr<Rom> &rom, int flags);
bool mount_rom(const std::shared_ptr<Rom> &rom);

}
