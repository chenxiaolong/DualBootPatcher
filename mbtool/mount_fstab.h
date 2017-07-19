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

#include "mbdevice/device.h"

#include "roms.h"

namespace mb
{

enum MountFlags : unsigned int
{
    // Rewrite fstab file to remove mounted entries
    MOUNT_FLAG_REWRITE_FSTAB            = 1u << 1,
    // Prevent the use of generic fstab entries for fstab files that are missing
    // entries for /system, /cache, or /data
    MOUNT_FLAG_NO_GENERIC_ENTRIES       = 1u << 2,
    // Unmount mount points that were successfully mounted if a later entry in
    // the fstab file fails to mount (affects only the mount points mounted in
    // the current invocation of the function)
    MOUNT_FLAG_UNMOUNT_ON_FAILURE       = 1u << 3,

    // Mount /system
    MOUNT_FLAG_MOUNT_SYSTEM             = 1u << 10,
    // Mount /cache
    MOUNT_FLAG_MOUNT_CACHE              = 1u << 11,
    // Mount /data
    MOUNT_FLAG_MOUNT_DATA               = 1u << 12,
    // Mount external SD
    MOUNT_FLAG_MOUNT_EXTERNAL_SD        = 1u << 13,
};

bool mount_fstab(const char *path, const std::shared_ptr<Rom> &rom,
                 Device *device, int flags);
bool mount_rom(const std::shared_ptr<Rom> &rom);

}
