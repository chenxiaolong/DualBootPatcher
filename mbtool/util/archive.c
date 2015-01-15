/*
 * Copyright (C) 2014  Xiao-Long Chen <chenxiaolong@cxl.epac.to>
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

#include "util/archive.h"

#include <errno.h>
#include <string.h>

#include <archive.h>
#include <archive_entry.h>

#include "logging.h"
#include "util/directory.h"

static int copy_data(struct archive *in, struct archive *out)
{
    const void *buff;
    size_t size;
    int64_t offset;
    int ret;

    while ((ret = archive_read_data_block(
            in, &buff, &size, &offset)) == ARCHIVE_OK) {
        if (archive_write_data_block(out, buff, size, offset) != ARCHIVE_OK) {
            LOGE("Failed to write data: %s", archive_error_string(out));
            return ARCHIVE_FAILED;
        }
    }

    if (ret != ARCHIVE_EOF) {
        LOGE("Data copy ended without reaching EOF: %s",
             archive_error_string(in));
        return ARCHIVE_FAILED;
    }

    return ARCHIVE_OK;
}

static int copy_header_and_data(struct archive *in, struct archive *out,
                                struct archive_entry *entry)
{
    int ret = ARCHIVE_OK;

    if ((ret = archive_write_header(out, entry)) != ARCHIVE_OK) {
        LOGE("Failed to write header: %s", archive_error_string(out));
        return ret;
    }

    if ((ret = copy_data(in, out)) != ARCHIVE_OK) {
        return ret;
    }

    return ret;
}

static int setup_input(struct archive **in, const char *filename)
{
    struct archive *a = NULL;
    a = archive_read_new();
    if (!a) {
        LOGE("Out of memory");
        return -1;
    }

    // Add more as needed
    //archive_read_support_format_all(a);
    //archive_read_support_filter_all(a);
    archive_read_support_format_tar(a);
    archive_read_support_format_zip(a);
    //archive_read_support_filter_xz(a);

    if (archive_read_open_filename(a, filename, 10240) != ARCHIVE_OK) {
        LOGE("%s: Failed to open archive: %s", filename, archive_error_string(a));
        goto error;
    }

    *in = a;
    return 0;

error:
    if (a) {
        archive_read_free(a);
    }
    return -1;
}

static int setup_output(struct archive **out)
{
    struct archive *a = NULL;
    a = archive_write_disk_new();
    if (!a) {
        LOGE("Out of memory");
        return -1;
    }

    archive_write_disk_set_options(a,
                                   ARCHIVE_EXTRACT_ACL |
                                   ARCHIVE_EXTRACT_FFLAGS |
                                   ARCHIVE_EXTRACT_PERM |
                                   ARCHIVE_EXTRACT_SECURE_NODOTDOT |
                                   ARCHIVE_EXTRACT_SECURE_SYMLINKS |
                                   ARCHIVE_EXTRACT_TIME |
                                   ARCHIVE_EXTRACT_UNLINK |
                                   ARCHIVE_EXTRACT_XATTR);

    *out = a;
    return 0;
}

int mb_extract_archive(const char *filename, const char *target)
{
    struct archive *in = NULL;
    struct archive *out = NULL;
    struct archive_entry *entry;
    int ret;
    char *cwd = NULL;

    if (setup_input(&in, filename) < 0) {
        goto error;
    }

    if (setup_output(&out) < 0) {
        goto error;
    }

    if (!(cwd = getcwd(NULL, 0))) {
        LOGE("Failed to get cwd: %s", strerror(errno));
        goto error;
    }

    if (mb_mkdir_recursive(
            target, S_IRWXU | S_IRWXG | S_IRWXO) < 0) {
        LOGE("%s: Failed to create directory: %s", target, strerror(errno));
        goto error;
    }

    if (chdir(target) < 0) {
        LOGE("%s: Failed to change to target directory: %s",
             target, strerror(errno));
        goto error;
    }

    while ((ret = archive_read_next_header(in, &entry)) == ARCHIVE_OK) {
        if (copy_header_and_data(in, out, entry) != ARCHIVE_OK) {
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

    return 0;

error:
    if (cwd) {
        chdir(cwd);
    }

    archive_read_free(in);
    archive_write_free(out);

    return -1;
}

int mb_extract_files(const char *filename, const char *target,
                     const char **files)
{
    struct archive *in = NULL;
    struct archive *out = NULL;
    struct archive_entry *entry;
    int ret;
    char *cwd = NULL;
    const char **files_ptr;
    int expected_count = 0;
    int count = 0;

    if (!files) {
        goto error;
    }

    for (files_ptr = files; *files_ptr; ++files_ptr) {
        ++expected_count;
    }

    if (setup_input(&in, filename) < 0) {
        goto error;
    }

    if (setup_output(&out) < 0) {
        goto error;
    }

    if (!(cwd = getcwd(NULL, 0))) {
        LOGE("Failed to get cwd: %s", strerror(errno));
        goto error;
    }

    if (mb_mkdir_recursive(
            target, S_IRWXU | S_IRWXG | S_IRWXO) < 0) {
        LOGE("%s: Failed to create directory: %s", target, strerror(errno));
        goto error;
    }

    if (chdir(target) < 0) {
        LOGE("%s: Failed to change to target directory: %s",
             target, strerror(errno));
        goto error;
    }

    while ((ret = archive_read_next_header(in, &entry)) == ARCHIVE_OK) {
        for (files_ptr = files; *files_ptr; ++files_ptr) {
            if (strcmp(archive_entry_pathname(entry), *files_ptr) == 0) {
                ++count;

                if (copy_header_and_data(in, out, entry) != ARCHIVE_OK) {
                    goto error;
                }
            }
        }
    }

    if (ret != ARCHIVE_EOF) {
        LOGE("Archive extraction ended without reaching EOF: %s",
             archive_error_string(in));
        goto error;
    }

    if (count != expected_count) {
        LOGE("Not all specified files were extracted");
        goto error;
    }

    chdir(cwd);

    archive_read_free(in);
    archive_write_free(out);

    return 0;

error:
    if (cwd) {
        chdir(cwd);
    }

    archive_read_free(in);
    archive_write_free(out);

    return -1;
}

int mb_extract_files2(const char *filename, const struct extract_info *files)
{
    struct archive *in = NULL;
    struct archive *out = NULL;
    struct archive_entry *entry;
    int ret;
    const struct extract_info *files_ptr;
    int expected_count = 0;
    int count = 0;

    if (!files) {
        goto error;
    }

    for (files_ptr = files; files_ptr->from && files_ptr->to; ++files_ptr) {
        ++expected_count;
    }

    if (setup_input(&in, filename) < 0) {
        goto error;
    }

    if (setup_output(&out) < 0) {
        goto error;
    }

    while ((ret = archive_read_next_header(in, &entry)) == ARCHIVE_OK) {
        for (files_ptr = files; files_ptr->from && files_ptr->to; ++files_ptr) {
            if (strcmp(archive_entry_pathname(entry), files_ptr->from) == 0) {
                ++count;

                archive_entry_set_pathname(entry, files_ptr->to);

                if (copy_header_and_data(in, out, entry) != ARCHIVE_OK) {
                    goto error;
                }

                archive_entry_set_pathname(entry, files_ptr->from);
            }
        }
    }

    if (ret != ARCHIVE_EOF) {
        LOGE("Archive extraction ended without reaching EOF: %s",
             archive_error_string(in));
        goto error;
    }

    if (count != expected_count) {
        LOGE("Not all specified files were extracted");
        goto error;
    }

    archive_read_free(in);
    archive_write_free(out);

    return 0;

error:
    archive_read_free(in);
    archive_write_free(out);

    return -1;
}
