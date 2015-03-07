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
#include <memory>
#include <cerrno>
#include <cstring>

#include <archive.h>
#include <archive_entry.h>

#include "util/directory.h"
#include "util/finally.h"
#include "util/logging.h"
#include "util/path.h"

namespace mb
{
namespace util
{

typedef std::unique_ptr<archive, int (*)(archive *)> archive_ptr;

static int copy_data(struct archive *in, struct archive *out)
{
    const void *buff;
    size_t size;
    int64_t offset;
    int ret;

    while ((ret = archive_read_data_block(
            in, &buff, &size, &offset)) == ARCHIVE_OK) {
        if (archive_write_data_block(out, buff, size, offset) != ARCHIVE_OK) {
            LOGE("Failed to write data: {}", archive_error_string(out));
            return ARCHIVE_FAILED;
        }
    }

    if (ret != ARCHIVE_EOF) {
        LOGE("Data copy ended without reaching EOF: {}",
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
        LOGE("Failed to write header: {}", archive_error_string(out));
        return ret;
    }

    if ((ret = copy_data(in, out)) != ARCHIVE_OK) {
        return ret;
    }

    return ret;
}

static bool set_up_input(archive *in, const std::string &filename)
{
    // Add more as needed
    //archive_read_support_format_all(in);
    //archive_read_support_filter_all(in);
    archive_read_support_format_tar(in);
    archive_read_support_format_zip(in);
    //archive_read_support_filter_xz(in);

    if (archive_read_open_filename(in, filename.c_str(), 10240) != ARCHIVE_OK) {
        LOGE("{}: Failed to open archive: {}",
             filename, archive_error_string(in));
        return false;
    }

    return true;
}

static void set_up_output(archive *out)
{
    archive_write_disk_set_options(out,
                                   ARCHIVE_EXTRACT_ACL |
                                   ARCHIVE_EXTRACT_FFLAGS |
                                   ARCHIVE_EXTRACT_PERM |
                                   ARCHIVE_EXTRACT_SECURE_NODOTDOT |
                                   ARCHIVE_EXTRACT_SECURE_SYMLINKS |
                                   ARCHIVE_EXTRACT_TIME |
                                   ARCHIVE_EXTRACT_UNLINK |
                                   ARCHIVE_EXTRACT_XATTR);
}

bool extract_archive(const std::string &filename, const std::string &target)
{
    archive_ptr in(archive_read_new(), archive_read_free);
    archive_ptr out(archive_write_disk_new(), archive_write_free);

    if (!in || !out) {
        LOGE("Out of memory");
        return false;
    }

    struct archive_entry *entry;
    int ret;
    std::string cwd = get_cwd();

    if (cwd.empty()) {
        return false;
    }

    if (!set_up_input(in.get(), filename)) {
        return false;
    }

    set_up_output(out.get());

    if (!mkdir_recursive(target, S_IRWXU | S_IRWXG | S_IRWXO)) {
        LOGE("{}: Failed to create directory: {}",
             target, strerror(errno));
        return false;
    }

    if (chdir(target.c_str()) < 0) {
        LOGE("{}: Failed to change to target directory: {}",
             target, strerror(errno));
        return false;
    }

    auto chdir_back = finally([&] {
        chdir(cwd.c_str());
    });

    while ((ret = archive_read_next_header(in.get(), &entry)) == ARCHIVE_OK) {
        if (copy_header_and_data(in.get(), out.get(), entry) != ARCHIVE_OK) {
            return false;
        }
    }

    if (ret != ARCHIVE_EOF) {
        LOGE("Archive extraction ended without reaching EOF: {}",
             archive_error_string(in.get()));
        return false;
    }

    return true;
}

bool extract_files(const std::string &filename, const std::string &target,
                   const std::vector<std::string> &files)
{
    if (files.empty()) {
        return false;
    }

    archive_ptr in(archive_read_new(), archive_read_free);
    archive_ptr out(archive_write_disk_new(), archive_write_free);

    if (!in || !out) {
        LOGE("Out of memory");
        return false;
    }

    struct archive_entry *entry;
    int ret;
    std::string cwd = get_cwd();
    unsigned int count = 0;

    if (cwd.empty()) {
        return false;
    }

    if (!set_up_input(in.get(), filename)) {
        return false;
    }

    set_up_output(out.get());

    if (!mkdir_recursive(target, S_IRWXU | S_IRWXG | S_IRWXO)) {
        LOGE("{}: Failed to create directory: {}",
             target, strerror(errno));
        return false;
    }

    if (chdir(target.c_str()) < 0) {
        LOGE("{}: Failed to change to target directory: {}",
             target, strerror(errno));
        return false;
    }

    auto chdir_back = finally([&] {
        chdir(cwd.c_str());
    });

    while ((ret = archive_read_next_header(in.get(), &entry)) == ARCHIVE_OK) {
        if (std::find(files.begin(), files.end(),
                archive_entry_pathname(entry)) != files.end()) {
            ++count;

            if (copy_header_and_data(in.get(), out.get(), entry) != ARCHIVE_OK) {
                return false;
            }
        }
    }

    if (ret != ARCHIVE_EOF) {
        LOGE("Archive extraction ended without reaching EOF: {}",
             archive_error_string(in.get()));
        return false;
    }

    if (count != files.size()) {
        LOGE("Not all specified files were extracted");
        return false;
    }

    return true;
}

bool extract_files2(const std::string &filename,
                    const std::vector<extract_info> &files)
{
    if (files.empty()) {
        return false;
    }

    archive_ptr in(archive_read_new(), archive_read_free);
    archive_ptr out(archive_write_disk_new(), archive_write_free);

    if (!in || !out) {
        LOGE("Out of memory");
        return false;
    }

    struct archive_entry *entry;
    int ret;
    unsigned int count = 0;

    if (!set_up_input(in.get(), filename)) {
        return false;
    }

    set_up_output(out.get());

    while ((ret = archive_read_next_header(in.get(), &entry)) == ARCHIVE_OK) {
        for (const extract_info &info : files) {
            if (info.from == archive_entry_pathname(entry)) {
                ++count;

                archive_entry_set_pathname(entry, info.to.c_str());

                if (copy_header_and_data(in.get(), out.get(), entry) != ARCHIVE_OK) {
                    return false;
                }

                archive_entry_set_pathname(entry, info.from.c_str());
            }
        }
    }

    if (ret != ARCHIVE_EOF) {
        LOGE("Archive extraction ended without reaching EOF: {}",
             archive_error_string(in.get()));
        return false;
    }

    if (count != files.size()) {
        LOGE("Not all specified files were extracted");
        return false;
    }

    return true;
}

bool archive_exists(const std::string &filename,
                    std::vector<exists_info> &files)
{
    if (files.empty()) {
        return false;
    }

    archive_ptr in(archive_read_new(), archive_read_free);

    if (!in) {
        LOGE("Out of memory");
        return false;
    }

    struct archive_entry *entry;
    int ret;

    for (exists_info &info : files) {
        info.exists = false;
    }

    if (!set_up_input(in.get(), filename)) {
        return false;
    }

    while ((ret = archive_read_next_header(in.get(), &entry)) == ARCHIVE_OK) {
        for (exists_info &info : files) {
            if (info.path == archive_entry_pathname(entry)) {
                info.exists = true;
            }
        }
    }

    if (ret != ARCHIVE_EOF) {
        LOGE("Archive extraction ended without reaching EOF: {}",
             archive_error_string(in.get()));
        return false;
    }

    return true;
}

}
}
