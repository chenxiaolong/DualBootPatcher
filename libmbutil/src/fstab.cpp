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

#include "mbcommon/finally.h"
#include "mbcommon/integer.h"
#include "mblog/logging.h"
#include "mbutil/string.h"

#define LOG_TAG "mbutil/fstab"


namespace mb
{
namespace util
{

using ScopedFILE = std::unique_ptr<FILE, decltype(fclose) *>;

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

static unsigned long options_to_flags(const MountFlag *flags_map,
                                      std::string args,
                                      std::string *new_args_out)
{
    unsigned long flags = 0;
    char *save_ptr;

    if (new_args_out) {
        new_args_out->clear();
    }

    char *temp = strtok_r(&args[0], ",", &save_ptr);
    while (temp) {
        const MountFlag *it;

        for (it = flags_map; it->name; ++it) {
            if (strncmp(temp, it->name, strlen(it->name)) == 0) {
                flags |= it->flag;
                break;
            }
        }

        if (!it->name) {
            if (new_args_out) {
                *new_args_out += temp;
                *new_args_out += ',';
            } else {
                LOGW("Only universal mount options expected, but found %s", temp);
            }
        }

        temp = strtok_r(nullptr, ",", &save_ptr);
    }

    if (new_args_out && !new_args_out->empty()) {
        new_args_out->pop_back();
    }

    return flags;
}

// Much simplified version of fs_mgr's fstab parsing code
std::vector<FstabRec> read_fstab(const std::string &path)
{
    ScopedFILE fp(fopen(path.c_str(), "rb"), fclose);
    if (!fp) {
        LOGE("Failed to open file %s: %s", path.c_str(), strerror(errno));
        return {};
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

        // Strip leading
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

        if ((temp = strtok_r(line, delim, &save_ptr)) == nullptr) {
            LOGE("No source path/device found in entry: %s", line);
            return {};
        }
        rec.blk_device = temp;

        if ((temp = strtok_r(nullptr, delim, &save_ptr)) == nullptr) {
            LOGE("No mount point found in entry: %s", line);
            return {};
        }
        rec.mount_point = temp;

        if ((temp = strtok_r(nullptr, delim, &save_ptr)) == nullptr) {
            LOGE("No filesystem type found in entry: %s", line);
            return {};
        }
        rec.fs_type = temp;

        if ((temp = strtok_r(nullptr, delim, &save_ptr)) == nullptr) {
            LOGE("No mount options found in entry: %s", line);
            return {};
        }
        rec.mount_args = temp;
        rec.flags = options_to_flags(g_mount_flags, temp, &rec.fs_options);

        if ((temp = strtok_r(nullptr, delim, &save_ptr)) == nullptr) {
            LOGE("No fs_mgr/vold options found in entry: %s", line);
            return {};
        }
        rec.vold_args = temp;
        rec.fs_mgr_flags = options_to_flags(g_fs_mgr_flags, temp, nullptr);

        fstab.push_back(std::move(rec));
    }

    return fstab;
}

std::vector<TwrpFstabRec> read_twrp_fstab(const std::string &path)
{
    ScopedFILE fp(fopen(path.c_str(), "rb"), fclose);
    if (!fp) {
        LOGE("Failed to open file %s: %s", path.c_str(), strerror(errno));
        return {};
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

        if ((temp = strtok_r(line, delim, &save_ptr)) == nullptr) {
            LOGE("No mount point found in entry: %s", line);
            return {};
        }
        rec.mount_point = temp;

        if ((temp = strtok_r(nullptr, delim, &save_ptr)) == nullptr) {
            LOGE("No filesystem type found in entry: %s", line);
            return {};
        }
        rec.fs_type = temp;

        if ((temp = strtok_r(nullptr, delim, &save_ptr)) == nullptr) {
            LOGE("No block device found in entry: %s", line);
            return {};
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
                    LOGE("Invalid length: %s", temp);
                    return {};
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

    return fstab;
}

}
}
