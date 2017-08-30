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
#include <vector>

namespace mb
{
namespace util
{

constexpr unsigned long MF_WAIT             = 1 << 0;
constexpr unsigned long MF_CHECK            = 1 << 1;
constexpr unsigned long MF_CRYPT            = 1 << 2;
constexpr unsigned long MF_NONREMOVABLE     = 1 << 3;
constexpr unsigned long MF_VOLDMANAGED      = 1 << 4;
constexpr unsigned long MF_LENGTH           = 1 << 5;
constexpr unsigned long MF_RECOVERYONLY     = 1 << 6;
constexpr unsigned long MF_SWAPPRIO         = 1 << 7;
constexpr unsigned long MF_ZRAMSIZE         = 1 << 8;
constexpr unsigned long MF_VERIFY           = 1 << 9;
constexpr unsigned long MF_FORCECRYPT       = 1 << 10;
constexpr unsigned long MF_NOEMULATEDSD     = 1 << 11;
constexpr unsigned long MF_NOTRIM           = 1 << 12;
constexpr unsigned long MF_FILEENCRYPTION   = 1 << 13;
constexpr unsigned long MF_FORMATTABLE      = 1 << 14;
constexpr unsigned long MF_SLOTSELECT       = 1 << 15;

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
