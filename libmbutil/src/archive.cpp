/*
 * Copyright (C) 2014-2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbutil/archive.h"

#include <algorithm>
#include <memory>
#include <cerrno>
#include <cstring>

#include "mblog/logging.h"
#include "mbutil/autoclose/archive.h"
#include "mbutil/directory.h"
#include "mbutil/finally.h"
#include "mbutil/path.h"

#define LIBARCHIVE_DISK_WRITER_FLAGS \
    ARCHIVE_EXTRACT_TIME \
    | ARCHIVE_EXTRACT_SECURE_SYMLINKS \
    | ARCHIVE_EXTRACT_SECURE_NODOTDOT \
    | ARCHIVE_EXTRACT_OWNER \
    | ARCHIVE_EXTRACT_PERM \
    | ARCHIVE_EXTRACT_ACL \
    | ARCHIVE_EXTRACT_XATTR \
    | ARCHIVE_EXTRACT_FFLAGS \
    | ARCHIVE_EXTRACT_MAC_METADATA \
    | ARCHIVE_EXTRACT_SPARSE

#define LIBARCHIVE_DISK_READER_FLAGS \
    ARCHIVE_READDISK_MAC_COPYFILE

namespace mb
{
namespace util
{

int libarchive_copy_data(archive *in, archive *out, archive_entry *entry)
{
    const void *buff;
    size_t size;
    int64_t offset;
    int ret;

    while ((ret = archive_read_data_block(
            in, &buff, &size, &offset)) == ARCHIVE_OK) {
        if (archive_write_data_block(out, buff, size, offset) != ARCHIVE_OK) {
            LOGE("%s: Failed to write data: %s", archive_entry_pathname(entry),
                 archive_error_string(out));
            return ARCHIVE_FAILED;
        }
    }

    if (ret != ARCHIVE_EOF) {
        LOGE("%s: Data copy ended without reaching EOF: %s",
             archive_entry_pathname(entry), archive_error_string(in));
        return ARCHIVE_FAILED;
    }

    return ARCHIVE_OK;
}

/*!
 * \brief Copy sparse file on disk to an archive
 *
 * \see tar/write.c from libarchive's source code
 */
bool libarchive_copy_data_disk_to_archive(archive *in, archive *out,
                                          archive_entry *entry)
{
    size_t bytes_read;
    ssize_t bytes_written;
    int64_t offset;
    int64_t progress = 0;
    char null_buf[64 * 1024];
    const void *buf;
    int ret;

    memset(null_buf, 0, sizeof(null_buf));

    while ((ret = archive_read_data_block(
            in, &buf, &bytes_read, &offset)) == ARCHIVE_OK) {
        if (offset > progress) {
            int64_t sparse = offset - progress;
            size_t ns;

            while (sparse > 0) {
                if (sparse > (int64_t) sizeof(null_buf)) {
                    ns = sizeof(null_buf);
                } else {
                    ns = (size_t) sparse;
                }

                bytes_written = archive_write_data(out, null_buf, ns);
                if (bytes_written < 0) {
                    LOGE("%s: %s", archive_entry_pathname(entry),
                         archive_error_string(out));
                    return false;
                }

                if ((size_t) bytes_written < ns) {
                    LOGE("%s: Truncated write", archive_entry_pathname(entry));
                    return false;
                }

                progress += bytes_written;
                sparse -= bytes_written;
            }
        }

        bytes_written = archive_write_data(out, buf, bytes_read);
        if (bytes_written < 0) {
            LOGE("%s: %s", archive_entry_pathname(entry),
                 archive_error_string(out));
            return false;
        }

        if ((size_t) bytes_written < bytes_read) {
            LOGE("%s: Truncated write", archive_entry_pathname(entry));
            return false;
        }

        progress += bytes_written;
    }

    if (ret != ARCHIVE_EOF) {
        LOGE("%s: %s", archive_entry_pathname(entry), archive_error_string(in));
        return false;
    }

    return true;
}

int libarchive_copy_header_and_data(archive *in, archive *out,
                                    archive_entry *entry)
{
    int ret = ARCHIVE_OK;

    if ((ret = archive_write_header(out, entry)) != ARCHIVE_OK) {
        LOGE("Failed to write header: %s", archive_error_string(out));
        return ret;
    }

    if ((ret = libarchive_copy_data(in, out, entry)) != ARCHIVE_OK) {
        return ret;
    }

    return ret;
}

/*
 * The following libarchive functions are based on code from bsdtar. The main
 * difference is that they will not try to extract/add as many files as possible
 * from/to the archive. They'll immediately fail after the first error or
 * warning because an incomplete archive is useless for backup and restoring.
 */

bool libarchive_tar_extract(const std::string &filename,
                            const std::string &target,
                            const std::vector<std::string> &patterns,
                            compression_type compression)
{
    if (target.empty()) {
        LOGE("%s: Invalid target path for extraction", target.c_str());
        return false;
    }

    autoclose::archive matcher(archive_match_new(), archive_match_free);
    if (!matcher) {
        LOGE("%s: Out of memory when creating matcher", __FUNCTION__);
        return false;
    }
    autoclose::archive in(archive_read_new(), archive_read_free);
    if (!in) {
        LOGE("%s: Out of memory when creating archive reader", __FUNCTION__);
        return false;
    }
    autoclose::archive out(archive_write_disk_new(), archive_write_free);
    if (!out) {
        LOGE("%s: Out of memory when creating disk writer", __FUNCTION__);
        return false;
    }

    // Set up matcher parameters
    for (const std::string &pattern : patterns) {
        if (archive_match_include_pattern(
                matcher.get(), pattern.c_str()) != ARCHIVE_OK) {
            LOGE("Invalid pattern: %s", pattern.c_str());
            return false;
        }
    }

    // Set up archive reader parameters
    //archive_read_support_format_gnutar(in.get());
    archive_read_support_format_tar(in.get());

    switch (compression) {
    case compression_type::NONE:
        break;
    case compression_type::LZ4:
        archive_read_support_filter_lz4(in.get());
        break;
    case compression_type::GZIP:
        archive_read_support_filter_gzip(in.get());
        break;
    case compression_type::XZ:
        archive_read_support_filter_xz(in.get());
        break;
    default:
        LOGE("Invalid compression type");
        return false;
    }

    // Set up disk writer parameters
    archive_write_disk_set_standard_lookup(out.get());
    archive_write_disk_set_options(out.get(), LIBARCHIVE_DISK_WRITER_FLAGS);

    if (archive_read_open_filename(
            in.get(), filename.c_str(), 10240) != ARCHIVE_OK) {
        LOGE("%s: Failed to open file: %s",
             filename.c_str(), archive_error_string(in.get()));
        return false;
    }

    archive_entry *entry;
    int ret;
    std::string target_path;

    while (true) {
        ret = archive_read_next_header(in.get(), &entry);
        if (ret == ARCHIVE_EOF) {
            break;
        } else if (ret == ARCHIVE_RETRY) {
            LOGW("%s: Retrying header read", filename.c_str());
            continue;
        } else if (ret != ARCHIVE_OK) {
            LOGE("%s: Failed to read header: %s",
                 filename.c_str(), archive_error_string(in.get()));
            return false;
        }

        const char *path = archive_entry_pathname(entry);
        if (!path || !*path) {
            LOGE("%s: Header has null or empty filename", filename.c_str());
            return false;
        }

        LOGV("%s", path);

        // Build path
        target_path = target;
        if (target_path.back() != '/' && *path != '/') {
            target_path += '/';
        }
        target_path += path;

        archive_entry_set_pathname(entry, target_path.c_str());

        // Check pattern matches
        if (archive_match_excluded(matcher.get(), entry)) {
            continue;
        }

        // Extract file
        ret = archive_read_extract2(in.get(), entry, out.get());
        if (ret != ARCHIVE_OK) {
            LOGE("%s: %s", archive_entry_pathname(entry),
                 archive_error_string(in.get()));
            return false;
        }
    }

    if (archive_read_close(in.get()) != ARCHIVE_OK) {
        LOGE("%s: %s", filename.c_str(), archive_error_string(in.get()));
        return false;
    }

    // Check that all patterns were matched
    const char *pattern;
    while ((ret = archive_match_path_unmatched_inclusions_next(
            matcher.get(), &pattern)) == ARCHIVE_OK) {
        LOGE("%s: Pattern not matched: %s", filename.c_str(), pattern);
    }
    if (ret != ARCHIVE_EOF) {
        LOGE("%s: %s", filename.c_str(), archive_error_string(matcher.get()));
        return false;
    }

    return archive_match_path_unmatched_inclusions(matcher.get()) == 0;
}

static bool write_file(archive *in, archive *out, archive_entry *entry)
{
    int ret;

    ret = archive_write_header(out, entry);
    if (ret != ARCHIVE_OK) {
        LOGE("%s: %s", archive_entry_pathname(entry), archive_error_string(out));
        return false;
    }

    if (archive_entry_size(entry) > 0) {
        return util::libarchive_copy_data_disk_to_archive(in, out, entry);
    }

    return true;
}

static int metadata_filter(archive *a, void *data, archive_entry *entry)
{
    (void) data;
    (void) entry;

    if (archive_read_disk_can_descend(a)) {
        archive_read_disk_descend(a);
    }
    return 1;
}

/*!
 * \brief Create pax archive with all metadata
 *
 * \param filename Target archive path
 * \param base_dir Base directory for \a paths
 * \param paths List of paths to add to the archive
 *
 * \return Whether the archive creation was successful
 */
bool libarchive_tar_create(const std::string &filename,
                           const std::string &base_dir,
                           const std::vector<std::string> &paths,
                           compression_type compression)
{
    if (base_dir.empty() && paths.empty()) {
        LOGE("%s: No base directory or paths specified", filename.c_str());
        return false;
    }

    autoclose::archive in(archive_read_disk_new(), archive_read_free);
    if (!in) {
        LOGE("%s: Out of memory when creating disk reader", __FUNCTION__);
        return false;
    }
    autoclose::archive out(archive_write_new(), archive_write_free);
    if (!out) {
        LOGE("%s: Out of memory when creating archive writer", __FUNCTION__);
        return false;
    }
    autoclose::archive_entry_linkresolver resolver(archive_entry_linkresolver_new(),
                                                   archive_entry_linkresolver_free);
    if (!resolver) {
        LOGE("%s: Out of memory when creating link resolver", __FUNCTION__);
        return false;
    }

    // Set up disk reader parameters
    archive_read_disk_set_symlink_physical(in.get());
    archive_read_disk_set_metadata_filter_callback(
            in.get(), metadata_filter, nullptr);
    archive_read_disk_set_behavior(in.get(), LIBARCHIVE_DISK_READER_FLAGS);
    // We don't want to look up usernames and group names on Android
    //archive_read_disk_set_standard_lookup(in.get());

    // Set up archive writer parameters
    // NOTE: We are creating POSIX pax archives instead of GNU tar archives
    //       because libarchive's GNU tar writer is very limited. In particular,
    //       it does not support storing sparse file information, xattrs, or
    //       ACLs. Since this information is stored as extended attributes in
    //       the pax archive, the GNU tar tool will not be able to extract any
    //       of this additional metadata. In other words, extracting and
    //       repacking a backup on a Linux machine with GNU tar will render the
    //       backup useless.
    //archive_write_set_format_gnutar(out.get());
    archive_write_set_format_pax_restricted(out.get());
    archive_write_set_bytes_per_block(out.get(), 10240);

    switch (compression) {
    case compression_type::NONE:
        break;
    case compression_type::LZ4:
        archive_write_add_filter_lz4(out.get());
        break;
    case compression_type::GZIP:
        archive_write_add_filter_gzip(out.get());
        break;
    case compression_type::XZ:
        archive_write_add_filter_xz(out.get());
        break;
    default:
        LOGE("Invalid compression type");
        return false;
    }

    // Set up link resolver parameters
    archive_entry_linkresolver_set_strategy(resolver.get(),
                                            archive_format(out.get()));

    // Open output file
    if (archive_write_open_filename(out.get(), filename.c_str()) != ARCHIVE_OK) {
        LOGE("%s: Failed to open file: %s",
             filename.c_str(), archive_error_string(out.get()));
        return false;
    }

    archive_entry *entry = nullptr;
    archive_entry *sparse_entry = nullptr;
    int ret;
    std::string full_path;

    // Add hierarchies
    for (const std::string &path : paths) {
        if (path.empty()) {
            LOGE("%s: Cannot add empty path to the archive", filename.c_str());
            return false;
        }

        // If the path is absolute, don't append it to the base directory
        if (path[0] == '/') {
            full_path = path;
        } else {
            full_path = base_dir;
            if (!full_path.empty() && full_path.back() != '/' && path[0] != '/') {
                full_path += '/';
            }
            full_path += path;
        }

        ret = archive_read_disk_open(in.get(), full_path.c_str());
        if (ret != ARCHIVE_OK) {
            LOGE("%s: %s", full_path.c_str(), archive_error_string(in.get()));
            return false;
        }

        while (true) {
            archive_entry_free(entry);
            entry = archive_entry_new();

            ret = archive_read_next_header2(in.get(), entry);
            if (ret == ARCHIVE_EOF) {
                break;
            } else if (ret != ARCHIVE_OK) {
                LOGE("%s: Failed to read next header: %s", full_path.c_str(),
                     archive_error_string(in.get()));
                archive_entry_free(entry);
                return false;
            }

            if (archive_entry_filetype(entry) != AE_IFREG) {
                archive_entry_set_size(entry, 0);
            }

            // If our current directory tree path is not an absolute path, set
            // the archive path to the relative path starting at base_dir
            const char *curpath = archive_entry_pathname(entry);
            if (curpath && path[0] != '/' && !base_dir.empty()) {
                std::string relpath;
                if (!util::relative_path(curpath, base_dir, &relpath)) {
                    LOGE("Failed to compute relative path of %s starting at %s: %s",
                         curpath, base_dir.c_str(), strerror(errno));
                    archive_entry_free(entry);
                    return false;
                }
                if (relpath.empty()) {
                    // If the relative path is empty, then the current path is
                    // the root of the directory tree. We don't need that, so
                    // skip it.
                    continue;
                }
                archive_entry_set_pathname(entry, relpath.c_str());
            }

            switch (archive_entry_filetype(entry)) {
            case AE_IFSOCK:
                LOGW("%s: Skipping socket", archive_entry_pathname(entry));
                continue;
            default:
                LOGV("%s", archive_entry_pathname(entry));
                break;
            }

            archive_entry_linkify(resolver.get(), &entry, &sparse_entry);

            if (entry) {
                if (!write_file(in.get(), out.get(), entry)) {
                    archive_entry_free(entry);
                    return false;
                }
                archive_entry_free(entry);
                entry = nullptr;
            }
            if (sparse_entry) {
                if (!write_file(in.get(), out.get(), sparse_entry)) {
                    archive_entry_free(sparse_entry);
                    return false;
                }
                archive_entry_free(sparse_entry);
                sparse_entry = nullptr;
            }
        }

        archive_entry_free(entry);
        entry = nullptr;
        archive_read_close(in.get());
    }

    archive_read_disk_set_metadata_filter_callback(in.get(), nullptr, nullptr);

    entry = nullptr;
    archive_entry_linkify(resolver.get(), &entry, &sparse_entry);

    while (entry) {
        // This tricky code here is to correctly read the contents of the entry
        // because the disk reader 'in' is pointing at does not have any
        // information about the entry by this time and using
        // archive_read_data_block() with the disk reader consequently must
        // fail. And we hae to re-open the entry to read the contents.
        ret = archive_read_disk_open(in.get(), archive_entry_sourcepath(entry));
        if (ret != ARCHIVE_OK) {
            LOGE("%s: %s", archive_entry_sourcepath(entry),
                 archive_error_string(in.get()));
            return false;
        }

        // Invoke archive_read_next_header2() to work archive_read_data_block(),
        // which is called via write_file() without failure.
        archive_entry *entry2 = archive_entry_new();
        ret = archive_read_next_header2(in.get(), entry2);
        archive_entry_free(entry2);
        if (ret != ARCHIVE_OK) {
            LOGE("%s: %s", archive_entry_sourcepath(entry),
                 archive_error_string(in.get()));
            archive_entry_free(entry);
            return false;
        }

        if (!write_file(in.get(), out.get(), entry)) {
            archive_entry_free(entry);
            return false;
        }
        archive_entry_free(entry);
        archive_read_close(in.get());
        entry = nullptr;
        archive_entry_linkify(resolver.get(), &entry, &sparse_entry);
    }

    if (archive_write_close(out.get()) != ARCHIVE_OK) {
        LOGE("%s: %s", filename.c_str(), archive_error_string(out.get()));
        return false;
    }

    return true;
}

static bool set_up_input(archive *in, const std::string &filename)
{
    // Add more as needed
    //archive_read_support_format_all(in);
    //archive_read_support_filter_all(in);
    //archive_read_support_format_tar(in);
    archive_read_support_format_zip(in);
    //archive_read_support_filter_xz(in);

    if (archive_read_open_filename(in, filename.c_str(), 10240) != ARCHIVE_OK) {
        LOGE("%s: Failed to open archive: %s",
             filename.c_str(), archive_error_string(in));
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
    autoclose::archive in(archive_read_new(), archive_read_free);
    autoclose::archive out(archive_write_disk_new(), archive_write_free);

    if (!in || !out) {
        LOGE("Out of memory");
        return false;
    }

    archive_entry *entry;
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
        LOGE("%s: Failed to create directory: %s",
             target.c_str(), strerror(errno));
        return false;
    }

    if (chdir(target.c_str()) < 0) {
        LOGE("%s: Failed to change to target directory: %s",
             target.c_str(), strerror(errno));
        return false;
    }

    auto chdir_back = finally([&] {
        chdir(cwd.c_str());
    });

    while ((ret = archive_read_next_header(in.get(), &entry)) == ARCHIVE_OK) {
        if (libarchive_copy_header_and_data(in.get(), out.get(), entry) != ARCHIVE_OK) {
            return false;
        }
    }

    if (ret != ARCHIVE_EOF) {
        LOGE("Archive extraction ended without reaching EOF: %s",
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

    autoclose::archive in(archive_read_new(), archive_read_free);
    autoclose::archive out(archive_write_disk_new(), archive_write_free);

    if (!in || !out) {
        LOGE("Out of memory");
        return false;
    }

    archive_entry *entry;
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
        LOGE("%s: Failed to create directory: %s",
             target.c_str(), strerror(errno));
        return false;
    }

    if (chdir(target.c_str()) < 0) {
        LOGE("%s: Failed to change to target directory: %s",
             target.c_str(), strerror(errno));
        return false;
    }

    auto chdir_back = finally([&] {
        chdir(cwd.c_str());
    });

    while ((ret = archive_read_next_header(in.get(), &entry)) == ARCHIVE_OK) {
        if (std::find(files.begin(), files.end(),
                archive_entry_pathname(entry)) != files.end()) {
            ++count;

            if (libarchive_copy_header_and_data(in.get(), out.get(), entry) != ARCHIVE_OK) {
                return false;
            }
        }
    }

    if (ret != ARCHIVE_EOF) {
        LOGE("Archive extraction ended without reaching EOF: %s",
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

    autoclose::archive in(archive_read_new(), archive_read_free);
    autoclose::archive out(archive_write_disk_new(), archive_write_free);

    if (!in || !out) {
        LOGE("Out of memory");
        return false;
    }

    archive_entry *entry;
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

                if (libarchive_copy_header_and_data(in.get(), out.get(), entry) != ARCHIVE_OK) {
                    return false;
                }

                archive_entry_set_pathname(entry, info.from.c_str());
            }
        }
    }

    if (ret != ARCHIVE_EOF) {
        LOGE("Archive extraction ended without reaching EOF: %s",
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

    autoclose::archive in(archive_read_new(), archive_read_free);

    if (!in) {
        LOGE("Out of memory");
        return false;
    }

    archive_entry *entry;
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
        LOGE("Archive extraction ended without reaching EOF: %s",
             archive_error_string(in.get()));
        return false;
    }

    return true;
}

}
}
