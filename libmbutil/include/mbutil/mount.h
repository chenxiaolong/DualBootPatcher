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

#include <optional>
#include <string>

#include "mbcommon/outcome.h"

namespace mb::util
{

enum class MountError
{
    PathNotMounted = 1,
};

std::error_code make_error_code(MountError e);

const std::error_category & mount_error_category();

struct MountEntry
{
    //! [mountinfo[1]] ID
    std::optional<unsigned int> id;
    //! [mountinfo[2]] Parent
    std::optional<unsigned int> parent;
    //! [mountinfo[3]] Device number of mount point
    std::optional<dev_t> dev;
    //! [mountinfo[4]] Root directory for bind mount
    std::optional<std::string> root;
    //! [mountinfo[5], mounts[2]] Mount point
    std::string target;
    //! [mountinfo[6], mounts[4]] VFS options
    std::string vfs_options;
    //! [mountinfo[8], mounts[3]] Filesystem type
    std::string type;
    //! [mountinfo[9], mounts[1]] Source device
    std::string source;
    //! [mountinfo[10], mounts[4]] FS options
    std::string fs_options;
    //! [mounts[5]] Dump frequency in days
    std::optional<int> freq;
    //! [mounts[6]] Parallel fsck pass number
    std::optional<int> passno;
};

oc::result<std::vector<MountEntry>> get_mount_entries();

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
