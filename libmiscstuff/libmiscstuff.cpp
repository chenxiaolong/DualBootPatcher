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

#include "libmiscstuff.h"

#include <memory>

#include <errno.h>
#include <sys/stat.h>
#include <sys/vfs.h>

#include <archive.h>
#include <archive_entry.h>

#include <android/log.h>

#include "mblog/android_logger.h"
#include "mblog/logging.h"


extern "C" {

int64_t get_mnt_total_size(const char *mountpoint) {
    struct statfs sfs;

    if (statfs(mountpoint, &sfs) < 0) {
        return 0;
    }

    return sfs.f_bsize * sfs.f_blocks;
}

int64_t get_mnt_avail_size(const char *mountpoint) {
    struct statfs sfs;

    if (statfs(mountpoint, &sfs) < 0) {
        return 0;
    }

    return sfs.f_bsize * sfs.f_bavail;
}

bool is_same_file(const char *path1, const char *path2)
{
    struct stat sb1;
    struct stat sb2;

    if (stat(path1, &sb1) < 0 || stat(path2, &sb2) < 0) {
        return false;
    }

    return sb1.st_dev == sb2.st_dev && sb1.st_ino == sb2.st_ino;
}

bool extract_archive(const char *filename, const char *target)
{
    struct archive *in = NULL;
    struct archive *out = NULL;
    struct archive_entry *entry;
    int ret;
    char *cwd = NULL;

    if (!(in = archive_read_new())) {
        LOGE("Out of memory");
        goto error;
    }

    // Add more as needed
    //archive_read_support_format_all(in);
    //archive_read_support_filter_all(in);
    archive_read_support_format_tar(in);
    archive_read_support_format_zip(in);
    archive_read_support_filter_xz(in);

    if (archive_read_open_filename(in, filename, 10240) != ARCHIVE_OK) {
        LOGE("%s: Failed to open archive: %s", filename, archive_error_string(in));
        goto error;
    }

    if (!(out = archive_write_disk_new())) {
        LOGE("Out of memory");
        goto error;
    }

    archive_write_disk_set_options(out,
                                   ARCHIVE_EXTRACT_ACL |
                                   ARCHIVE_EXTRACT_FFLAGS |
                                   ARCHIVE_EXTRACT_PERM |
                                   ARCHIVE_EXTRACT_SECURE_NODOTDOT |
                                   ARCHIVE_EXTRACT_SECURE_SYMLINKS |
                                   ARCHIVE_EXTRACT_TIME |
                                   ARCHIVE_EXTRACT_UNLINK |
                                   ARCHIVE_EXTRACT_XATTR);

    if (!(cwd = getcwd(NULL, 0))) {
        LOGE("Failed to get cwd: %s", strerror(errno));
        goto error;
    }

    if (chdir(target) < 0) {
        LOGE("%s: Failed to change to target directory: %s",
             target, strerror(errno));
        goto error;
    }

    while ((ret = archive_read_next_header(in, &entry)) == ARCHIVE_OK) {
        if ((ret = archive_write_header(out, entry)) != ARCHIVE_OK) {
            LOGE("Failed to write header: %s", archive_error_string(out));
            goto error;
        }

        const void *buff;
        size_t size;
        int64_t offset;
        int ret;

        while ((ret = archive_read_data_block(
                in, &buff, &size, &offset)) == ARCHIVE_OK) {
            if (archive_write_data_block(out, buff, size, offset) != ARCHIVE_OK) {
                LOGE("Failed to write data: %s", archive_error_string(out));
                goto error;
            }
        }

        if (ret != ARCHIVE_EOF) {
            LOGE("Data copy ended without reaching EOF: %s",
                 archive_error_string(in));
            goto error;
        }
    }

    if (ret != ARCHIVE_EOF) {
        LOGE("Archive extraction ended without reaching EOF: %s",
             archive_error_string(in));
        goto error;
    }

    chdir(cwd);

    archive_read_free(in);
    archive_write_free(out);

    return true;

error:
    if (cwd) {
        chdir(cwd);
    }

    archive_read_free(in);
    archive_write_free(out);

    return false;
}

void mblog_set_logcat()
{
    mb::log::set_log_tag("libmbp");
    mb::log::log_set_logger(std::make_shared<mb::log::AndroidLogger>());
}

}
