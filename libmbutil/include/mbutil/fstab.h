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
#include <vector>

#define MF_WAIT             0x1
#define MF_CHECK            0x2
#define MF_CRYPT            0x4
#define MF_NONREMOVABLE     0x8
#define MF_VOLDMANAGED      0x10
#define MF_LENGTH           0x20
#define MF_RECOVERYONLY     0x40
#define MF_SWAPPRIO         0x80
#define MF_ZRAMSIZE         0x100
#define MF_VERIFY           0x200
#define MF_FORCECRYPT       0x400
#define MF_NOEMULATEDSD     0x800
#define MF_NOTRIM           0x1000
#define MF_FILEENCRYPTION   0x2000
#define MF_FORMATTABLE      0x4000
#define MF_SLOTSELECT       0x8000

namespace mb
{
namespace util
{

struct fstab_rec
{
    // Block device to mount
    std::string blk_device;
    // Mount point
    std::string mount_point;
    // Filesystem type
    std::string fs_type;
    // Flags for mount()
    unsigned long flags;
    // Filesystem options for mount()
    std::string fs_options;
    // Flags for fs_mgr
    unsigned long fs_mgr_flags;
    // Options for vold
    std::string vold_args;
    // Original mount arguments from fstab
    std::string mount_args;
    // Original line from fstab file
    std::string orig_line;
};

struct twrp_fstab_rec
{
    std::vector<std::string> blk_devices;
    std::string mount_point;
    std::string fs_type;
    std::vector<std::string> twrp_flags;
    std::vector<std::string> unknown_options;
    int length;
    std::string orig_line;
};

std::vector<fstab_rec> read_fstab(const std::string &path);
std::vector<twrp_fstab_rec> read_twrp_fstab(const std::string &path);

}
}
