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

#include "mbcommon/flags.h"
#include "mbdevice/device.h"

#include "util/roms.h"

namespace android::init
{
    class DeviceHandler;
}

namespace mb
{

enum class MountFlag : uint32_t
{
    // Rewrite fstab file to remove mounted entries
    RewriteFstab        = 1u << 1,
    // Prevent the use of generic fstab entries for fstab files that are missing
    // entries for /system, /cache, or /data
    NoGenericEntries    = 1u << 2,
    // Unmount mount points that were successfully mounted if a later entry in
    // the fstab file fails to mount (affects only the mount points mounted in
    // the current invocation of the function)
    UnmountOnFailure    = 1u << 3,

    // Mount /system
    MountSystem         = 1u << 10,
    // Mount /cache
    MountCache          = 1u << 11,
    // Mount /data
    MountData           = 1u << 12,
    // Mount external SD
    MountExternalSd     = 1u << 13,
};
MB_DECLARE_FLAGS(MountFlags, MountFlag)
MB_DECLARE_OPERATORS_FOR_FLAGS(MountFlags)

bool mount_fstab(const char *path, const std::shared_ptr<Rom> &rom,
                 const device::Device &device, MountFlags flags,
                 const android::init::DeviceHandler &handler);
bool mount_rom(const std::shared_ptr<Rom> &rom);

}
