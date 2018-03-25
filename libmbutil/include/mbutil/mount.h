/*
 * Copyright (C) 2014-2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include <cstdio>

#include "mbcommon/outcome.h"

namespace mb::util
{

enum class MountError
{
    EndOfFile = 1,
    PathNotMounted,
};

std::error_code make_error_code(MountError e);

const std::error_category & mount_error_category();

constexpr char PROC_MOUNTS[] = "/proc/mounts";

struct MountEntry
{
    std::string fsname;
    std::string dir;
    std::string type;
    std::string opts;
    int freq;
    int passno;
};

oc::result<MountEntry> get_mount_entry(std::FILE *fp);

oc::result<void> is_mounted(const std::string &mountpoint);
oc::result<void> unmount_all(const std::string &dir);
oc::result<void> mount(const std::string &source, const std::string &target,
                       const std::string &fstype, unsigned long mount_flags,
                       const std::string &data);
oc::result<void> umount(const std::string &target);

oc::result<uint64_t> mount_get_total_size(const std::string &path);
oc::result<uint64_t> mount_get_avail_size(const std::string &path);

}

namespace std
{
    template<>
    struct is_error_code_enum<mb::util::MountError> : true_type
    {
    };
}
