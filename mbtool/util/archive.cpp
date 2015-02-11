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

#include "util/archive.h"

#include <algorithm>
#include <cerrno>
#include <cstring>

#include <archive.h>
#include <archive_entry.h>

#include "logging.h"
#include "util/directory.h"

namespace MB {

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

static bool setup_input(struct archive **in, const std::string &filename)
{
    struct archive *a = NULL;
    a = archive_read_new();
    if (!a) {
        LOGE("Out of memory");
        return false;
    }

    // Add more as needed
    //archive_read_support_format_all(a);
    //archive_read_support_filter_all(a);
    archive_read_support_format_tar(a);
    archive_read_support_format_zip(a);
    //archive_read_support_filter_xz(a);

    if (archive_read_open_filename(a, filename.c_str(), 10240) != ARCHIVE_OK) {
        LOGE("%s: Failed to open archive: %s",
             filename, archive_error_string(a));
        goto error;
    }

    *in = a;
    return true;

error:
    if (a) {
        archive_read_free(a);
    }
    return false;
}

static bool setup_output(struct archive **out)
{
    struct archive *a = NULL;
    a = archive_write_disk_new();
    if (!a) {
        LOGE("Out of memory");
        return false;
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
    return true;
}

bool extract_archive(const std::string &filename, const std::string &target)
{
    struct archive *in = NULL;
    struct archive *out = NULL;
    struct archive_entry *entry;
    int ret;
    char *cwd = NULL;

    if (!setup_input(&in, filename)) {
        goto error;
    }

    if (!setup_output(&out)) {
        goto error;
    }

    if (!(cwd = getcwd(NULL, 0))) {
        LOGE("Failed to get cwd: %s", strerror(errno));
        goto error;
    }

    if (!MB::mkdir_recursive(target, S_IRWXU | S_IRWXG | S_IRWXO)) {
        LOGE("%s: Failed to create directory: %s",
             target, strerror(errno));
        goto error;
    }

    if (chdir(target.c_str()) < 0) {
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

    return true;

error:
    if (cwd) {
        chdir(cwd);
    }

    archive_read_free(in);
    archive_write_free(out);

    return false;
}

bool extract_files(const std::string &filename, const std::string &target,
                   const std::vector<std::string> &files)
{
    struct archive *in = NULL;
    struct archive *out = NULL;
    struct archive_entry *entry;
    int ret;
    char *cwd = NULL;
    int expected_count = files.size();
    int count = 0;

    if (files.empty()) {
        goto error;
    }

    if (!setup_input(&in, filename)) {
        goto error;
    }

    if (!setup_output(&out)) {
        goto error;
    }

    if (!(cwd = getcwd(NULL, 0))) {
        LOGE("Failed to get cwd: %s", strerror(errno));
        goto error;
    }

    if (!MB::mkdir_recursive(target, S_IRWXU | S_IRWXG | S_IRWXO)) {
        LOGE("%s: Failed to create directory: %s",
             target, strerror(errno));
        goto error;
    }

    if (chdir(target.c_str()) < 0) {
        LOGE("%s: Failed to change to target directory: %s",
             target, strerror(errno));
        goto error;
    }

    while ((ret = archive_read_next_header(in, &entry)) == ARCHIVE_OK) {
        if (std::find(files.begin(), files.end(),
                archive_entry_pathname(entry)) != files.end()) {
            ++count;

            if (copy_header_and_data(in, out, entry) != ARCHIVE_OK) {
                goto error;
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

    return true;

error:
    if (cwd) {
        chdir(cwd);
    }

    archive_read_free(in);
    archive_write_free(out);

    return false;
}

bool extract_files2(const std::string &filename,
                    const std::vector<extract_info> &files)
{
    struct archive *in = NULL;
    struct archive *out = NULL;
    struct archive_entry *entry;
    int ret;
    int expected_count = files.size();
    int count = 0;

    if (files.empty()) {
        goto error;
    }

    if (!setup_input(&in, filename)) {
        goto error;
    }

    if (!setup_output(&out)) {
        goto error;
    }

    while ((ret = archive_read_next_header(in, &entry)) == ARCHIVE_OK) {
        for (const extract_info &info : files) {
            if (info.from == archive_entry_pathname(entry)) {
                ++count;

                archive_entry_set_pathname(entry, info.to.c_str());

                if (copy_header_and_data(in, out, entry) != ARCHIVE_OK) {
                    goto error;
                }

                archive_entry_set_pathname(entry, info.from.c_str());
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

    return true;

error:
    archive_read_free(in);
    archive_write_free(out);

    return false;
}

bool archive_exists(const std::string &filename,
                    std::vector<exists_info> &files)
{
    struct archive *in = NULL;
    struct archive_entry *entry;
    int ret;

    if (files.empty()) {
        goto error;
    }

    for (exists_info &info : files) {
        info.exists = false;
    }

    if (!setup_input(&in, filename)) {
        goto error;
    }

    while ((ret = archive_read_next_header(in, &entry)) == ARCHIVE_OK) {
        for (exists_info &info : files) {
            if (info.path == archive_entry_pathname(entry)) {
                info.exists = true;
            }
        }
    }

    if (ret != ARCHIVE_EOF) {
        LOGE("Archive extraction ended without reaching EOF: %s",
             archive_error_string(in));
        goto error;
    }

    archive_read_free(in);

    return true;

error:
    archive_read_free(in);

    return false;
}

}