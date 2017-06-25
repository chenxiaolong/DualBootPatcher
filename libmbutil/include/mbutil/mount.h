/*
 * Copyright (C) 2014  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#define PROC_MOUNTS "/proc/mounts"

namespace mb
{
namespace util
{

struct MountEntry
{
    std::string fsname;
    std::string dir;
    std::string type;
    std::string opts;
    int freq;
    int passno;
};

bool get_mount_entry(std::FILE *fp, MountEntry &entry_out);

bool is_mounted(const std::string &mountpoint);
bool unmount_all(const std::string &dir);
bool mount(const char *source, const char *target, const char *fstype,
           unsigned long mount_flags, const void *data);
bool umount(const char *target);

uint64_t mount_get_total_size(const char *path);
uint64_t mount_get_avail_size(const char *path);

}
}
