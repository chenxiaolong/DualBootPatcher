/*
 * Copyright (C) 2014  Andrew Gunnerson <andrewgunnerson@gmail.com>
 *
 * This file is part of MultiBootPatcher
 *
 * MultiBootPatcher is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MultiBootPatcher is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MultiBootPatcher.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "util/fstab.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctype.h>
#include <sys/mount.h>

#include "logging.h"


namespace MB {

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
    { "ro",             0 },
    { "defaults",       0 },
    { NULL,             0 }
};


static int options_to_flags(char *args, char *new_args, int size);


// Much simplified version of fs_mgr's fstab parsing code
std::vector<fstab_rec> read_fstab(const std::string &path)
{
    std::FILE *fp;
    int count, entries;
    char *line = NULL;
    size_t len = 0; // allocated memory size
    ssize_t bytes_read; // number of bytes read
    char *temp;
    char *save_ptr;
    const char *delim = " \t";
    std::vector<fstab_rec> fstab;
    char temp_mount_args[1024];

    fp = std::fopen(path.c_str(), "rb");
    if (!fp) {
        LOGE("Failed to open file %s", path.c_str());
        return std::vector<fstab_rec>();
    }

    entries = 0;
    while ((bytes_read = getline(&line, &len, fp)) != -1) {
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
        goto error;
    }

    std::fseek(fp, 0, SEEK_SET);

    count = 0;
    while ((bytes_read = getline(&line, &len, fp)) != -1) {
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

        if ((temp = strtok_r(line, delim, &save_ptr)) == NULL) {
            LOGE("No source path/device found in entry: %s", line);
            goto error;
        }
        rec.blk_device = temp;

        if ((temp = strtok_r(NULL, delim, &save_ptr)) == NULL) {
            LOGE("No mount point found in entry: %s", line);
            goto error;
        }
        rec.mount_point = temp;

        if ((temp = strtok_r(NULL, delim, &save_ptr)) == NULL) {
            LOGE("No filesystem type found in entry: %s", line);
            goto error;
        }
        rec.fs_type = temp;

        if ((temp = strtok_r(NULL, delim, &save_ptr)) == NULL) {
            LOGE("No mount options found in entry: %s", line);
            goto error;
        }
        rec.flags = options_to_flags(temp, temp_mount_args, 1024);

        if (temp_mount_args[0]) {
            rec.fs_options = temp_mount_args;
        }

        if ((temp = strtok_r(NULL, delim, &save_ptr)) == NULL) {
            LOGE("No fs_mgr/vold options found in entry: %s", line);
            goto error;
        }
        rec.vold_args = temp;

        fstab.push_back(std::move(rec));

        ++count;
    }

    std::fclose(fp);
    free(line);
    return fstab;

error:
    std::fclose(fp);
    free(line);
    return std::vector<fstab_rec>();
}

static int options_to_flags(char *args, char *new_args, int size)
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
        for (i = 0; mount_flags[i].name; ++i) {
            if (strcmp(temp, mount_flags[i].name) == 0) {
                flags |= 0;
                break;
            }
        }

        if (!mount_flags[i].name) {
            if (new_args) {
                strlcat(new_args, temp, size);
                strlcat(new_args, ",", size);
            } else {
                LOGW("Only universal mount options expected, but found %s", temp);
            }
        }

        temp = strtok_r(NULL, ",", &save_ptr);
    }

    if (new_args && new_args[0]) {
        new_args[strlen(new_args) - 1] = '\0';
    }

    return flags;
}

}