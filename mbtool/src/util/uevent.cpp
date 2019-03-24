/*
 * Copyright (C) 2019  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "util/uevent.h"

#include <cerrno>
#include <cstring>

#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <unistd.h>

#include "mbcommon/finally.h"

#include "mblog/logging.h"

#include "mbutil/path.h"
#include "mbutil/properties.h"

#define LOG_TAG "mbtool/util/uevent"

namespace mb
{

//! Get major/minor number from uevent file in sysfs
static std::optional<dev_t> get_device_number(const char *uevent_path)
{
    std::optional<dev_t> major;
    std::optional<dev_t> minor;

    util::wait_for_path(uevent_path, std::chrono::seconds(20));

    if (!util::property_file_iter(uevent_path, {},
            [&](std::string_view key, std::string_view value) {
        dev_t v;

        if (key == "MAJOR" && str_to_num(value, 10, v)) {
            major = v;
        } else if (key == "MINOR" && str_to_num(value, 10, v)) {
            minor = v;
        }
        return util::PropertyIterAction::Continue;
    })) {
        LOGE("%s: Failed to load properties", uevent_path);
        return std::nullopt;
    }

    if (!major || !minor) {
        LOGE("%s: Invalid major/minor numbers", uevent_path);
        return std::nullopt;
    }

    return makedev(*major, *minor);
}

//! Create a block device corresponding to a uevent path
bool create_block_dev(const char *uevent_path, const char *block_dev)
{
    // Mount /sys
    mkdir("/sys", 0755);
    mount("sysfs", "/sys", "sysfs", 0, nullptr);

    // Unmount /sys
    auto clean_up_sys = finally([&] {
        umount("/sys");
        rmdir("/sys");
    });

    auto dev_num = get_device_number(block_dev);
    if (!dev_num) {
        return false;
    }

    if (mknod(uevent_path, S_IFBLK, *dev_num) < 0) {
        LOGE("%s: Failed to create block device: %s",
             uevent_path, strerror(errno));
        return false;
    }

    return true;
}

}
