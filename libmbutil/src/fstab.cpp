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

#include "mbutil/fstab.h"

#include <memory>

#include <cctype>
#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <sys/mount.h>

#include "mblog/logging.h"
#include "mbutil/autoclose/file.h"
#include "mbutil/finally.h"
#include "mbutil/string.h"


namespace mb
{
namespace util
{

struct mount_flag
{
    const char *name;
    int flag;
};

static struct mount_flag mount_flags[] =
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
    { "nouser",         MS_NOUSER },
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

static struct mount_flag fs_mgr_flags[] =
{
    { "wait",           MF_WAIT },
    { "check",          MF_CHECK },
    { "encryptable=",   MF_CRYPT },
    { "forceencrypt=",  MF_FORCECRYPT },
    { "fileencryption", MF_FILEENCRYPTION },
    { "nonremovable",   MF_NONREMOVABLE },
    { "voldmanaged=",   MF_VOLDMANAGED},
    { "length=",        MF_LENGTH },
    { "recoveryonly",   MF_RECOVERYONLY },
    { "swapprio=",      MF_SWAPPRIO },
    { "zramsize=",      MF_ZRAMSIZE },
    { "verify",         MF_VERIFY },
    { "noemulatedsd",   MF_NOEMULATEDSD },
    { "notrim",         MF_NOTRIM },
    { "formattable",    MF_FORMATTABLE },
    { "slotselect",     MF_SLOTSELECT },
    { "defaults",       0 },
    { nullptr,          0 }
};

static int options_to_flags(struct mount_flag *flags_map, char *args,
                            char *new_args, int size)
{
    char *temp;
    char *save_ptr;
    int flags = 0;
    int i;

    if (new_args && size > 0) {
        new_args[0] = '\0';
    }

    temp = strtok_r(args, ",", &save_ptr);
    while (temp) {
        for (i = 0; flags_map[i].name; ++i) {
            if (strncmp(temp, flags_map[i].name, strlen(flags_map[i].name)) == 0) {
                flags |= flags_map[i].flag;
                break;
            }
        }

        if (!flags_map[i].name) {
            if (new_args) {
                strlcat(new_args, temp, size);
                strlcat(new_args, ",", size);
            } else {
                LOGW("Only universal mount options expected, but found %s", temp);
            }
        }

        temp = strtok_r(nullptr, ",", &save_ptr);
    }

    if (new_args && new_args[0]) {
        new_args[strlen(new_args) - 1] = '\0';
    }

    return flags;
}

// Much simplified version of fs_mgr's fstab parsing code
std::vector<fstab_rec> read_fstab(const std::string &path)
{
    autoclose::file fp(autoclose::fopen(path.c_str(), "rb"));
    if (!fp) {
        LOGE("Failed to open file %s: %s", path.c_str(), strerror(errno));
        return std::vector<fstab_rec>();
    }

    int count, entries;
    char *line = nullptr;
    size_t len = 0; // allocated memory size
    ssize_t bytes_read; // number of bytes read
    char *temp;
    char *save_ptr;
    const char *delim = " \t";
    std::vector<fstab_rec> fstab;
    char temp_mount_args[1024];

    auto free_line = finally([&] {
        free(line);
    });

    entries = 0;
    while ((bytes_read = getline(&line, &len, fp.get())) != -1) {
        // Strip newlines
        if (bytes_read > 0 && line[bytes_read - 1] == '\n') {
            line[bytes_read - 1] = '\0';
        }

        // Strip leading
        temp = line;
        while (isspace(*temp)) {
            ++temp;
        }

        // Skip empty lines and comments
        if (*temp == '\0' || *temp == '#') {
            continue;
        }

        ++entries;
    }

    if (entries == 0) {
        LOGE("fstab contains no entries");
        return std::vector<fstab_rec>();
    }

    std::fseek(fp.get(), 0, SEEK_SET);

    count = 0;
    while ((bytes_read = getline(&line, &len, fp.get())) != -1) {
        // Strip newlines
        if (bytes_read > 0 && line[bytes_read - 1] == '\n') {
            line[bytes_read - 1] = '\0';
        }

        // Strip leading
        temp = line;
        while (isspace(*temp)) {
            ++temp;
        }

        // Skip empty lines and comments
        if (*temp == '\0' || *temp == '#') {
            continue;
        }

        // Avoid possible overflow if the file was changed
        if (count >= entries) {
            LOGE("Found more fstab entries on second read than first read");
            break;
        }

        fstab_rec rec;

        rec.orig_line = line;

        if ((temp = strtok_r(line, delim, &save_ptr)) == nullptr) {
            LOGE("No source path/device found in entry: %s", line);
            return std::vector<fstab_rec>();
        }
        rec.blk_device = temp;

        if ((temp = strtok_r(nullptr, delim, &save_ptr)) == nullptr) {
            LOGE("No mount point found in entry: %s", line);
            return std::vector<fstab_rec>();
        }
        rec.mount_point = temp;

        if ((temp = strtok_r(nullptr, delim, &save_ptr)) == nullptr) {
            LOGE("No filesystem type found in entry: %s", line);
            return std::vector<fstab_rec>();
        }
        rec.fs_type = temp;

        if ((temp = strtok_r(nullptr, delim, &save_ptr)) == nullptr) {
            LOGE("No mount options found in entry: %s", line);
            return std::vector<fstab_rec>();
        }
        rec.mount_args = temp;
        rec.flags = options_to_flags(mount_flags, temp, temp_mount_args, 1024);

        if (temp_mount_args[0]) {
            rec.fs_options = temp_mount_args;
        }

        if ((temp = strtok_r(nullptr, delim, &save_ptr)) == nullptr) {
            LOGE("No fs_mgr/vold options found in entry: %s", line);
            return std::vector<fstab_rec>();
        }
        rec.vold_args = temp;
        rec.fs_mgr_flags = options_to_flags(fs_mgr_flags, temp, nullptr, 0);

        fstab.push_back(std::move(rec));

        ++count;
    }

    return fstab;
}

static bool convert_to_int(const char *str, int *out)
{
    char *end;
    errno = 0;
    long num = strtol(str, &end, 10);
    if (errno == ERANGE || num < INT_MIN || num > INT_MAX
            || *str == '\0' || *end != '\0') {
        return false;
    }
    *out = (int) num;
    return true;
}

std::vector<twrp_fstab_rec> read_twrp_fstab(const std::string &path)
{
    autoclose::file fp(autoclose::fopen(path.c_str(), "rb"));
    if (!fp) {
        LOGE("Failed to open file %s: %s", path.c_str(), strerror(errno));
        return {};
    }

    char *line = nullptr;
    size_t len = 0; // allocated memory size
    ssize_t bytes_read; // number of bytes read
    char *temp;
    char *save_ptr;
    const char *delim = " \t";
    std::vector<twrp_fstab_rec> fstab;

    auto free_line = finally([&] {
        free(line);
    });

    while ((bytes_read = getline(&line, &len, fp.get())) != -1) {
        // Strip newlines
        if (bytes_read > 0 && line[bytes_read - 1] == '\n') {
            line[bytes_read - 1] = '\0';
        }

        // Strip leading whitespace
        temp = line;
        while (isspace(*temp)) {
            ++temp;
        }

        // Skip empty lines and comments
        if (*temp == '\0' || *temp == '#') {
            continue;
        }

        twrp_fstab_rec rec;

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
                if (!convert_to_int(temp, &rec.length)) {
                    LOGE("Invalid length: %s", temp);
                    return {};
                }
            } else if (strncmp(temp, "flags=", 6) == 0) {
                // TWRP flags
                temp += 6;
                rec.twrp_flags = util::tokenize(temp, ";");
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
