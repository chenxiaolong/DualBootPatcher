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

#include "mbutil/fstab.h"

#include <memory>

#include <cctype>
#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <sys/mount.h>

#include "mbcommon/error_code.h"
#include "mbcommon/finally.h"
#include "mbcommon/integer.h"
#include "mbcommon/string.h"
#include "mblog/logging.h"
#include "mbutil/string.h"

#define LOG_TAG "mbutil/fstab"


namespace mb::util
{

using ScopedFILE = std::unique_ptr<FILE, decltype(fclose) *>;

struct FstabErrorCategory : std::error_category
{
    const char * name() const noexcept override;

    std::string message(int ev) const override;
};

const std::error_category & fstab_error_category()
{
    static FstabErrorCategory c;
    return c;
}

std::error_code make_error_code(FstabError e)
{
    return {static_cast<int>(e), fstab_error_category()};
}

const char * FstabErrorCategory::name() const noexcept
{
    return "fstab";
}

std::string FstabErrorCategory::message(int ev) const
{
    switch (static_cast<FstabError>(ev)) {
    case FstabError::MissingSourcePath:
        return "missing source path";
    case FstabError::MissingTargetPath:
        return "missing target path";
    case FstabError::MissingFilesystemType:
        return "missing filesystem type";
    case FstabError::MissingMountOptions:
        return "missing mount options";
    case FstabError::MissingVoldOptions:
        return "missing vold options";
    case FstabError::InvalidLength:
        return "invalid length";
    default:
        return "(unknown fstab error)";
    }
}

std::string FstabErrorInfo::message() const
{
    std::string buf;

    if (!line.empty()) {
        buf += "invalid line: '";
        buf += line;
        buf += "': ";
    }

    buf += ec.message();

    return buf;
}

struct MountFlag
{
    const char *name;
    unsigned long flag;
};

static struct MountFlag g_mount_flags[] =
{
    { "active",         MS_ACTIVE },
    { "bind",           MS_BIND },
    { "dirsync",        MS_DIRSYNC },
    { "mandlock",       MS_MANDLOCK },
    { "move",           MS_MOVE },
    { "noatime",        MS_NOATIME },
    { "nodev",          MS_NODEV },
    { "nodiratime",     MS_NODIRATIME },
    { "noexec",         MS_NOEXEC },
    { "nosuid",         MS_NOSUID },
    { "nouser",         static_cast<unsigned long>(MS_NOUSER) }, // 1 << 31
    { "posixacl",       MS_POSIXACL },
    { "rec",            MS_REC },
    { "ro",             MS_RDONLY },
    { "relatime",       MS_RELATIME },
    { "remount",        MS_REMOUNT },
    { "silent",         MS_SILENT },
    { "strictatime",    MS_STRICTATIME },
    { "sync",           MS_SYNCHRONOUS },
    { "unbindable",     MS_UNBINDABLE },
    { "private",        MS_PRIVATE },
    { "slave",          MS_SLAVE },
    { "shared",         MS_SHARED },
    // Flags that should be ignored
    { "rw",             0 },
    { "defaults",       0 },
    { nullptr,          0 }
};

static struct MountFlag g_fs_mgr_flags[] =
{
    { "wait",               MF_WAIT },
    { "check",              MF_CHECK },
    { "encryptable=",       MF_CRYPT },
    { "forceencrypt=",      MF_FORCECRYPT },
    { "fileencryption=",    MF_FILEENCRYPTION },
    { "forcefdeorfbe=",     MF_FORCEFDEORFBE },
    { "nonremovable",       MF_NONREMOVABLE },
    { "voldmanaged=",       MF_VOLDMANAGED},
    { "length=",            MF_LENGTH },
    { "recoveryonly",       MF_RECOVERYONLY },
    { "swapprio=",          MF_SWAPPRIO },
    { "zramsize=",          MF_ZRAMSIZE },
    { "max_comp_streams=",  MF_MAX_COMP_STREAMS },
    { "verifyatboot",       MF_VERIFYATBOOT },
    { "verify",             MF_VERIFY },
    { "avb",                MF_AVB },
    { "noemulatedsd",       MF_NOEMULATEDSD },
    { "notrim",             MF_NOTRIM },
    { "formattable",        MF_FORMATTABLE },
    { "slotselect",         MF_SLOTSELECT },
    { "nofail",             MF_NOFAIL },
    { "latemount",          MF_LATEMOUNT },
    { "reservedsize=",      MF_RESERVEDSIZE },
    { "quota",              MF_QUOTA },
    { "eraseblk=",          MF_ERASEBLKSIZE },
    { "logicalblk=",        MF_LOGICALBLKSIZE },
    { "defaults",           0 },
    { nullptr,              0 },
};

static std::pair<unsigned long, std::string>
options_to_flags(const MountFlag *flags_map, std::string options)
{
    unsigned long flags = 0;
    std::string new_options;
    char *save_ptr;

    char *temp = strtok_r(options.data(), ",", &save_ptr);
    while (temp) {
        const MountFlag *it;

        for (it = flags_map; it->name; ++it) {
            if (starts_with(temp, it->name)) {
                flags |= it->flag;
                break;
            }
        }

        if (!it->name) {
            new_options += temp;
            new_options += ',';
        }

        temp = strtok_r(nullptr, ",", &save_ptr);
    }

    if (!new_options.empty()) {
        new_options.pop_back();
    }

    return {flags, std::move(new_options)};
}

// Much simplified version of fs_mgr's fstab parsing code
FstabResult<FstabRecs> read_fstab(const std::string &path)
{
    ScopedFILE fp(fopen(path.c_str(), "rb"), fclose);
    if (!fp) {
        return FstabErrorInfo{{}, ec_from_errno()};
    }

    char *line = nullptr;
    size_t len = 0; // allocated memory size
    ssize_t bytes_read; // number of bytes read
    constexpr char delim[] = " \t";
    std::vector<FstabRec> fstab;

    auto free_line = finally([&] {
        free(line);
    });

    while ((bytes_read = getline(&line, &len, fp.get())) != -1) {
        // Strip newlines
        if (bytes_read > 0 && line[bytes_read - 1] == '\n') {
            line[bytes_read - 1] = '\0';
        }

        // Strip leading whitespace
        char *temp = line;
        while (isspace(*temp)) {
            ++temp;
        }

        // Skip empty lines and comments
        if (*temp == '\0' || *temp == '#') {
            continue;
        }

        FstabRec rec;
        char *save_ptr;

        rec.orig_line = line;

        if (!(temp = strtok_r(line, delim, &save_ptr))) {
            return FstabErrorInfo{line, FstabError::MissingSourcePath};
        }
        rec.blk_device = temp;

        if (!(temp = strtok_r(nullptr, delim, &save_ptr))) {
            return FstabErrorInfo{line, FstabError::MissingTargetPath};
        }
        rec.mount_point = temp;

        if (!(temp = strtok_r(nullptr, delim, &save_ptr))) {
            return FstabErrorInfo{line, FstabError::MissingFilesystemType};
        }
        rec.fs_type = temp;

        if (!(temp = strtok_r(nullptr, delim, &save_ptr))) {
            return FstabErrorInfo{line, FstabError::MissingMountOptions};
        }
        rec.mount_args = temp;
        std::tie(rec.flags, rec.fs_options) =
                options_to_flags(g_mount_flags, temp);

        if (!(temp = strtok_r(nullptr, delim, &save_ptr))) {
            return FstabErrorInfo{line, FstabError::MissingVoldOptions};
        }
        rec.vold_args = temp;
        std::tie(rec.fs_mgr_flags, std::ignore) =
                options_to_flags(g_fs_mgr_flags, temp);

        fstab.push_back(std::move(rec));
    }

    if (ferror(fp.get())) {
        return FstabErrorInfo{{}, ec_from_errno()};
    }

    return std::move(fstab);
}

FstabResult<TwrpFstabRecs> read_twrp_fstab(const std::string &path)
{
    ScopedFILE fp(fopen(path.c_str(), "rb"), fclose);
    if (!fp) {
        return FstabErrorInfo{{}, ec_from_errno()};
    }

    char *line = nullptr;
    size_t len = 0; // allocated memory size
    ssize_t bytes_read; // number of bytes read
    constexpr char delim[] = " \t";
    std::vector<TwrpFstabRec> fstab;

    auto free_line = finally([&] {
        free(line);
    });

    while ((bytes_read = getline(&line, &len, fp.get())) != -1) {
        // Strip newlines
        if (bytes_read > 0 && line[bytes_read - 1] == '\n') {
            line[bytes_read - 1] = '\0';
        }

        // Strip leading whitespace
        char *temp = line;
        while (isspace(*temp)) {
            ++temp;
        }

        // Skip empty lines and comments
        if (*temp == '\0' || *temp == '#') {
            continue;
        }

        TwrpFstabRec rec;
        char *save_ptr;

        rec.orig_line = line;

        if (!(temp = strtok_r(line, delim, &save_ptr))) {
            return FstabErrorInfo{line, FstabError::MissingTargetPath};
        }
        rec.mount_point = temp;

        if (!(temp = strtok_r(nullptr, delim, &save_ptr))) {
            return FstabErrorInfo{line, FstabError::MissingFilesystemType};
        }
        rec.fs_type = temp;

        if (!(temp = strtok_r(nullptr, delim, &save_ptr))) {
            return FstabErrorInfo{line, FstabError::MissingSourcePath};
        }
        rec.blk_devices.push_back(temp);

        while ((temp = strtok_r(nullptr, delim, &save_ptr))) {
            if (*temp == '/') {
                // Additional block device
                rec.blk_devices.push_back(temp);
            } else if (strncmp(temp, "length=", 7) == 0) {
                // Length of partition
                temp += 7;
                if (!str_to_num(temp, 10, rec.length)) {
                    return FstabErrorInfo{line, FstabError::InvalidLength};
                }
            } else if (strncmp(temp, "flags=", 6) == 0) {
                // TWRP flags
                temp += 6;
                rec.twrp_flags = tokenize(temp, ";");
            } else if (strncmp(temp, "null", 4) == 0
                    || strncmp(temp, "NULL", 4) == 0) {
                // Skip
                continue;
            } else {
                // Unknown option
                rec.unknown_options.push_back(temp);
            }
        }

        fstab.push_back(std::move(rec));
    }

    if (ferror(fp.get())) {
        return FstabErrorInfo{{}, ec_from_errno()};
    }

    return std::move(fstab);
}

}
