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

#include <string>
#include <vector>

#include "mbcommon/outcome.h"

namespace mb::util
{

enum class FstabError
{
    MissingSourcePath = 1,
    MissingTargetPath,
    MissingFilesystemType,
    MissingMountOptions,
    MissingVoldOptions,
    InvalidLength,
};

std::error_code make_error_code(FstabError e);

const std::error_category & fstab_error_category();

struct FstabErrorInfo
{
    std::string line;
    std::error_code ec;

    std::string message() const;
};

inline const std::error_code & make_error_code(const FstabErrorInfo &ei)
{
    return ei.ec;
}

[[noreturn]]
inline void outcome_throw_as_system_error_with_payload(const FstabErrorInfo &ei)
{
    (void) ei;
    OUTCOME_THROW_EXCEPTION(std::system_error(make_error_code(ei)));
}

template <typename T>
using FstabResult = oc::result<T, FstabErrorInfo>;

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
constexpr unsigned long MF_FORCEFDEORFBE    = 1 << 16;
constexpr unsigned long MF_LATEMOUNT        = 1 << 17;
constexpr unsigned long MF_NOFAIL           = 1 << 18;
constexpr unsigned long MF_VERIFYATBOOT     = 1 << 19;
constexpr unsigned long MF_MAX_COMP_STREAMS = 1 << 20;
constexpr unsigned long MF_RESERVEDSIZE     = 1 << 21;
constexpr unsigned long MF_QUOTA            = 1 << 22;
constexpr unsigned long MF_ERASEBLKSIZE     = 1 << 23;
constexpr unsigned long MF_LOGICALBLKSIZE   = 1 << 24;
constexpr unsigned long MF_AVB              = 1 << 25;

std::pair<unsigned long, std::vector<std::string>>
parse_mount_options(std::string_view options);
std::pair<unsigned long, std::vector<std::string>>
parse_fs_mgr_options(std::string_view options);

struct FstabRec
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

using FstabRecs = std::vector<FstabRec>;

struct TwrpFstabRec
{
    std::vector<std::string> blk_devices;
    std::string mount_point;
    std::string fs_type;
    std::vector<std::string> twrp_flags;
    std::vector<std::string> unknown_options;
    int length;
    std::string orig_line;
};

using TwrpFstabRecs = std::vector<TwrpFstabRec>;

FstabResult<FstabRecs> read_fstab(const std::string &path);
FstabResult<TwrpFstabRecs> read_twrp_fstab(const std::string &path);

}

namespace std
{
    template<>
    struct is_error_code_enum<mb::util::FstabError> : true_type
    {
    };
}
