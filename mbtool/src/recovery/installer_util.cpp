/*
 * Copyright (C) 2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "recovery/installer_util.h"

#include <memory>
#include <optional>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <fcntl.h>
#include <sys/stat.h>

#include <archive.h>
#include <archive_entry.h>

#include "mbbootimg/entry.h"
#include "mbbootimg/header.h"
#include "mbbootimg/reader.h"
#include "mbbootimg/writer.h"

#include "mbcommon/file.h"
#include "mbcommon/file_util.h"
#include "mbcommon/file/standard.h"
#include "mbcommon/finally.h"
#include "mbcommon/string.h"

#include "mblog/logging.h"

#include "mbutil/delete.h"
#include "mbutil/path.h"

#include "recovery/bootimg_util.h"
#include "util/multiboot.h"

#define LOG_TAG "mbtool/recovery/installer_util"

using namespace mb::bootimg;

typedef std::unique_ptr<archive, decltype(archive_free) *> ScopedArchive;
typedef std::unique_ptr<archive_entry, decltype(archive_entry_free) *> ScopedArchiveEntry;
typedef std::unique_ptr<FILE, decltype(fclose) *> ScopedFILE;

namespace mb
{

bool InstallerUtil::unpack_ramdisk(const std::string &input_file,
                                   const std::string &output_dir,
                                   int &format_out,
                                   std::vector<int> &filters_out)
{
    ScopedArchive ain(archive_read_new(), archive_read_free);
    ScopedArchive aout(archive_write_disk_new(), archive_write_free);

    if (!ain || !aout) {
        LOGE("Failed to allocate archive reader or writer instance");
        return false;
    }

    archive_read_support_filter_gzip(ain.get());
    archive_read_support_filter_lz4(ain.get());
    archive_read_support_filter_lzma(ain.get());
    archive_read_support_filter_xz(ain.get());
    archive_read_support_format_cpio(ain.get());

    // Set up disk writer parameters
    archive_write_disk_set_standard_lookup(aout.get());
    archive_write_disk_set_options(aout.get(),
                                   ARCHIVE_EXTRACT_TIME
                                 | ARCHIVE_EXTRACT_SECURE_SYMLINKS
                                 | ARCHIVE_EXTRACT_SECURE_NODOTDOT
                                 | ARCHIVE_EXTRACT_OWNER
                                 | ARCHIVE_EXTRACT_PERM);

    if (archive_read_open_filename(ain.get(), input_file.c_str(), 10240)
            != ARCHIVE_OK) {
        LOGE("%s: Failed to open for reading: %s",
             input_file.c_str(), archive_error_string(ain.get()));
        return false;
    }

    // Can't use O_PATH because we support devices running kernel <3.5
    int cwd_fd = open(".", O_DIRECTORY | O_CLOEXEC);
    if (cwd_fd < 0) {
        LOGE("Failed to open current directory: %s", strerror(errno));
        return false;
    }

    auto close_cwd_fd = finally([&] {
        if (fchdir(cwd_fd) < 0) {
            LOGW("Failed to change back to previous directory: %s",
                 strerror(errno));
        }

        close(cwd_fd);
    });

    if (chdir(output_dir.c_str()) < 0) {
        LOGE("%s: Failed to change directory: %s",
             output_dir.c_str(), strerror(errno));
        return false;
    }

    while (true) {
        archive_entry *entry;

        int ret = archive_read_next_header(ain.get(), &entry);
        if (ret == ARCHIVE_EOF) {
            break;
        } else if (ret == ARCHIVE_RETRY) {
            continue;
        } else if (ret != ARCHIVE_OK) {
            LOGE("%s: Failed to read header: %s",
                 input_file.c_str(), archive_error_string(ain.get()));
            return false;
        }

        const char *path = archive_entry_pathname(entry);
        if (!path || !*path) {
            LOGE("%s: Header has null or empty filename", input_file.c_str());
            return false;
        }

        // Don't allow absolute paths (libarchive will reject '..'s)
        while (*path && *path == '/') {
            ++path;
        }

        archive_entry_set_pathname(entry, path);

        // Extract file
        ret = archive_read_extract2(ain.get(), entry, aout.get());
        if (ret != ARCHIVE_OK) {
            LOGE("%s: Failed to extract file: %s", path,
                 archive_error_string(ain.get()));
            return false;
        }
    }

    // Save format
    format_out = archive_format(ain.get());
    filters_out.clear();
    for (int i = 0; i < archive_filter_count(ain.get()); ++i) {
        int code = archive_filter_code(ain.get(), i);
        if (code != ARCHIVE_FILTER_NONE) {
            filters_out.push_back(code);
        }
    }

    if (archive_read_close(ain.get()) != ARCHIVE_OK) {
        LOGE("%s: %s", input_file.c_str(), archive_error_string(ain.get()));
        return false;
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

bool InstallerUtil::pack_ramdisk(const std::string &input_dir,
                                 const std::string &output_file,
                                 int format,
                                 const std::vector<int> &filters)
{
    ScopedArchive ain(archive_read_disk_new(), archive_read_free);
    ScopedArchive aout(archive_write_new(), archive_write_free);
    ScopedArchiveEntry entry(archive_entry_new(), archive_entry_free);
    std::string full_path;
    int ret;

    if (!ain || !aout || !entry) {
        LOGE("Failed to allocate archive reader, writer, or entry instance");
        return false;
    }

    // Set up disk reader parameters
    archive_read_disk_set_symlink_physical(ain.get());
    archive_read_disk_set_metadata_filter_callback(
            ain.get(), metadata_filter, nullptr);
    // We don't want to look up usernames and group names on Android
    //archive_read_disk_set_standard_lookup(in.get());

    if (archive_write_set_format(aout.get(), format) != ARCHIVE_OK) {
        LOGE("Failed to set output archive format: %s",
             archive_error_string(aout.get()));
        return false;
    }
    for (const int &filter : filters) {
        if (archive_write_add_filter(aout.get(), filter) != ARCHIVE_OK) {
            LOGE("Failed to add output archive filter: %s",
                 archive_error_string(aout.get()));
            return false;
        }
    }

    archive_write_set_bytes_per_block(aout.get(), 512);

    // Open output file
    if (archive_write_open_filename(aout.get(), output_file.c_str())
            != ARCHIVE_OK) {
        LOGE("%s: Failed to open for writing: %s",
             output_file.c_str(), archive_error_string(aout.get()));
        return false;
    }

    ret = archive_read_disk_open(ain.get(), input_dir.c_str());
    if (ret != ARCHIVE_OK) {
        LOGE("%s: %s", input_dir.c_str(), archive_error_string(ain.get()));
        return false;
    }

    while (true) {
        // libarchive doesn't clear the entry automatically
        archive_entry_clear(entry.get());

        ret = archive_read_next_header2(ain.get(), entry.get());
        if (ret == ARCHIVE_EOF) {
            break;
        } else if (ret != ARCHIVE_OK) {
            LOGE("%s: Failed to read next header: %s", input_dir.c_str(),
                 archive_error_string(ain.get()));
            return false;
        }

        if (archive_entry_filetype(entry.get()) != AE_IFREG) {
            archive_entry_set_size(entry.get(), 0);
        }

        const char *curpath = archive_entry_pathname(entry.get());
        if (curpath && !input_dir.empty()) {
            auto relpath = util::relative_path(curpath, input_dir);
            if (!relpath) {
                LOGE("Failed to compute relative path of %s starting at %s: %s",
                     curpath, input_dir.c_str(),
                     relpath.error().message().c_str());
                return false;
            }
            if (relpath.value().empty()) {
                // If the relative path is empty, then the current path is
                // the root of the directory tree. We don't need that, so
                // skip it.
                continue;
            }
            archive_entry_set_pathname(entry.get(), relpath.value().c_str());
        }

        ret = archive_write_header(aout.get(), entry.get());
        if (ret != ARCHIVE_OK) {
            LOGE("%s: %s", output_file.c_str(),
                 archive_error_string(aout.get()));
            return false;
        }

        char buf[10240];
        la_ssize_t n;

        if (archive_entry_size(entry.get()) > 0) {
            while ((n = archive_read_data(ain.get(), buf, sizeof(buf))) > 0) {
                if (archive_write_data(aout.get(), buf, static_cast<size_t>(n))
                        != n) {
                    LOGE("Failed to write archive entry data: %s",
                         archive_error_string(aout.get()));
                    return false;
                }
            }

            if (n < 0) {
                LOGE("Failed to read archive entry data: %s",
                     archive_error_string(ain.get()));
                return false;
            }
        }
    }

    archive_read_close(ain.get());

    if (archive_write_close(aout.get()) != ARCHIVE_OK) {
        LOGE("%s: %s", output_file.c_str(), archive_error_string(aout.get()));
        return false;
    }

    return true;
}

bool InstallerUtil::patch_boot_image(const std::string &input_file,
                                     const std::string &output_file,
                                     const std::vector<std::function<RamdiskPatcherFn>> &rps)
{
    std::string tmpdir = format("%s.XXXXXX", output_file.c_str());

    // std::string is guaranteed to be contiguous in C++11
    if (!mkdtemp(tmpdir.data())) {
        LOGE("Failed to create temporary directory: %s", strerror(errno));
        return false;
    }

    auto delete_temp_dir = finally([&]{
        (void) util::delete_recursive(tmpdir);
    });

    Reader reader;
    Writer writer;
    Header header;
    Entry in_entry;
    Entry out_entry;

    // Debug
    LOGD("Patching boot image");
    LOGD("- Input: %s", input_file.c_str());
    LOGD("- Output: %s", output_file.c_str());

    // Open input boot image
    auto ret = reader.enable_format_all();
    if (!ret) {
        LOGE("Failed to enable input boot image formats: %s",
             ret.error().message().c_str());
        return false;
    }
    ret = reader.open_filename(input_file);
    if (!ret) {
        LOGE("%s: Failed to open boot image for reading: %s",
             input_file.c_str(), ret.error().message().c_str());
        return false;
    }

    // Open output boot image
    ret = writer.set_format_by_code(reader.format_code());
    if (!ret) {
        LOGE("Failed to set output boot image format: %s",
             ret.error().message().c_str());
        return false;
    }
    ret = writer.open_filename(output_file);
    if (!ret) {
        LOGE("%s: Failed to open boot image for writing: %s",
             output_file.c_str(), ret.error().message().c_str());
        return false;
    }

    // Debug
    LOGD("- Format: %s", reader.format_name().c_str());

    // Copy header
    ret = reader.read_header(header);
    if (!ret) {
        LOGE("%s: Failed to read header: %s",
             input_file.c_str(), ret.error().message().c_str());
        return false;
    }
    ret = writer.write_header(header);
    if (!ret) {
        LOGE("%s: Failed to write header: %s",
             output_file.c_str(), ret.error().message().c_str());
        return false;
    }

    // Write entries
    while (true) {
        ret = writer.get_entry(out_entry);
        if (!ret) {
            if (ret.error() == WriterError::EndOfEntries) {
                break;
            }
            LOGE("%s: Failed to get entry: %s",
                 output_file.c_str(), ret.error().message().c_str());
            return false;
        }

        auto type = out_entry.type();
        LOGD("%s: Writer is requesting entry type: %d",
             output_file.c_str(), *type);

        // Write entry metadata
        ret = writer.write_entry(out_entry);
        if (!ret) {
            LOGE("%s: Failed to write entry: %s",
                 output_file.c_str(), ret.error().message().c_str());
            return false;
        }

        // Special case for loki aboot
        if (*type == ENTRY_TYPE_ABOOT) {
            LOGD("%s: Copying aboot partition: %s",
                 output_file.c_str(), ABOOT_PARTITION);

            if (bi_copy_file_to_data(ABOOT_PARTITION, writer)) {
                return false;
            }
        } else {
            ret = reader.go_to_entry(in_entry, *type);
            if (!ret) {
                if (ret.error() == ReaderError::EndOfEntries) {
                    LOGV("%s: Skipping non-existent boot image entry: %d",
                         input_file.c_str(), *type);
                    continue;
                } else {
                    LOGE("%s: Failed to go to entry: %d: %s",
                         input_file.c_str(), *type,
                         ret.error().message().c_str());
                    return false;
                }
            }

            if (type == ENTRY_TYPE_RAMDISK) {
                LOGD("%s: Writing patched ramdisk", output_file.c_str());

                std::string ramdisk_in(tmpdir);
                ramdisk_in += "/ramdisk.in";
                std::string ramdisk_out(tmpdir);
                ramdisk_out += "/ramdisk.out";

                auto delete_temp_files = finally([&]{
                    unlink(ramdisk_in.c_str());
                    unlink(ramdisk_out.c_str());
                });

                if (!bi_copy_data_to_file(reader, ramdisk_in)) {
                    return false;
                }

                if (!patch_ramdisk(ramdisk_in, ramdisk_out, 0, rps)) {
                    return false;
                }

                if (!bi_copy_file_to_data(ramdisk_out, writer)) {
                    return false;
                }
            } else if (type == ENTRY_TYPE_KERNEL) {
                LOGD("%s: Writing patched kernel", output_file.c_str());

                std::string kernel_in(tmpdir);
                kernel_in += "/kernel.in";
                std::string kernel_out(tmpdir);
                kernel_out += "/kernel.out";

                auto delete_temp_files = finally([&]{
                    unlink(kernel_in.c_str());
                    unlink(kernel_out.c_str());
                });

                if (!bi_copy_data_to_file(reader, kernel_in)) {
                    return false;
                }

                if (!patch_kernel_rkp(kernel_in, kernel_out)) {
                    return false;
                }

                if (!bi_copy_file_to_data(kernel_out, writer)) {
                    return false;
                }
            } else {
                LOGD("%s: Copying entry directly", output_file.c_str());

                // Copy entry directly
                if (!bi_copy_data_to_data(reader, writer)) {
                    return false;
                }
            }
        }
    }

    ret = writer.close();
    if (!ret) {
        LOGE("%s: Failed to close boot image: %s",
             output_file.c_str(), ret.error().message().c_str());
        return false;
    }

    return true;
}

bool InstallerUtil::patch_ramdisk(const std::string &input_file,
                                  const std::string &output_file,
                                  unsigned int depth,
                                  const std::vector<std::function<RamdiskPatcherFn>> &rps)
{
    if (depth > 1) {
        LOGV("Ignoring doubly-nested ramdisk");
        return true;
    }

    std::string tmpdir = format("%s.XXXXXX", output_file.c_str());

    if (!mkdtemp(tmpdir.data())) {
        LOGE("Failed to create temporary directory: %s", strerror(errno));
        return false;
    }

    auto delete_temp_dir = finally([&]{
        (void) util::delete_recursive(tmpdir);
    });

    int format;
    std::vector<int> filters;

    // Extract ramdisk
    if (!unpack_ramdisk(input_file, tmpdir, format, filters)) {
        return false;
    }

    // Patch ramdisk
    std::string nested(tmpdir);
    nested += "/sbin/ramdisk.cpio";
    struct stat sb;
    bool ret;

    if (stat(nested.c_str(), &sb) == 0) {
        ret = patch_ramdisk(nested, nested, depth + 1, rps);
    } else {
        ret = patch_ramdisk_dir(tmpdir, rps);
    }

    // Pack ramdisk
    if (!pack_ramdisk(tmpdir, output_file, format, filters)) {
        return false;
    }

    return ret;
}

bool InstallerUtil::patch_ramdisk_dir(const std::string &ramdisk_dir,
                                      const std::vector<std::function<RamdiskPatcherFn>> &rps)
{
    for (auto const &rp : rps) {
        if (!rp(ramdisk_dir)) {
            return false;
        }
    }

    return true;
}

bool InstallerUtil::patch_kernel_rkp(const std::string &input_file,
                                     const std::string &output_file)
{
    // We'll use SuperSU's patch for negating the effects of
    // CONFIG_RKP_NS_PROT=y in newer Samsung kernels. This kernel feature
    // prevents exec()'ing anything as a privileged user unless the binary
    // resides in rootfs or whichever filesystem was first mounted at /system.
    //
    // It is trivial to update mbtool to only use rootfs, but we need to
    // override fsck tools by bind-mounting dummy binaries from an ext4 image
    // when booting from an external SD. Unless we patch the SELinux policy to
    // allow vold to execute u:r:rootfs:s0-labeled fsck binaries, this patch
    // must remain.

    static const unsigned char source_pattern[] = {
        0x49, 0x01, 0x00, 0x54, 0x01, 0x14, 0x40, 0xB9, 0x3F, 0xA0,
        0x0F, 0x71, 0xE9, 0x00, 0x00, 0x54, 0x01, 0x08, 0x40, 0xB9,
        0x3F, 0xA0, 0x0F, 0x71, 0x89, 0x00, 0x00, 0x54, 0x00, 0x18,
        0x40, 0xB9, 0x1F, 0xA0, 0x0F, 0x71, 0x88, 0x01, 0x00, 0x54,
    };
    static const unsigned char target_pattern[] = {
        0xA1, 0x02, 0x00, 0x54, 0x01, 0x14, 0x40, 0xB9, 0x3F, 0xA0,
        0x0F, 0x71, 0x40, 0x02, 0x00, 0x54, 0x01, 0x08, 0x40, 0xB9,
        0x3F, 0xA0, 0x0F, 0x71, 0xE0, 0x01, 0x00, 0x54, 0x00, 0x18,
        0x40, 0xB9, 0x1F, 0xA0, 0x0F, 0x71, 0x81, 0x01, 0x00, 0x54,
    };

    StandardFile fin;
    StandardFile fout;
    std::optional<uint64_t> offset;

    // Open input file
    auto open_ret = fin.open(input_file, FileOpenMode::ReadOnly);
    if (!open_ret) {
        LOGE("%s: Failed to open for reading: %s",
             input_file.c_str(), open_ret.error().message().c_str());
        return false;
    }

    // Open output file
    open_ret = fout.open(output_file, FileOpenMode::WriteOnly);
    if (!open_ret) {
        LOGE("%s: Failed to open for writing: %s",
             output_file.c_str(), open_ret.error().message().c_str());
        return false;
    }

    // Replace pattern
    auto result_cb = [&](File &file, uint64_t offset_)
            -> oc::result<FileSearchAction> {
        (void) file;
        offset = offset_;
        return FileSearchAction::Stop;
    };

    auto search_ret = file_search(fin, {}, {}, 0, source_pattern,
                                  sizeof(source_pattern), 1, result_cb);
    if (!search_ret) {
        LOGE("%s: Error when searching for pattern: %s",
             input_file.c_str(), search_ret.error().message().c_str());
        return false;
    }

    // Copy data
    auto seek_ret = fin.seek(0, SEEK_SET);
    if (!seek_ret) {
        LOGE("%s: Failed to seek to beginning: %s",
             input_file.c_str(), seek_ret.error().message().c_str());
        return false;
    }

    if (offset) {
        LOGD("RKP pattern found at offset: 0x%" PRIx64, *offset);

        if (!copy_file_to_file(fin, fout, *offset)) {
            return false;
        }

        seek_ret = fin.seek(sizeof(source_pattern), SEEK_CUR);
        if (!seek_ret) {
            LOGE("%s: Failed to skip pattern: %s",
                 input_file.c_str(), seek_ret.error().message().c_str());
            return false;
        }

        auto ret = file_write_exact(fout, target_pattern,
                                    sizeof(target_pattern));
        if (!ret) {
            LOGE("%s: Failed to write target pattern: %s",
                 output_file.c_str(), ret.error().message().c_str());
            return false;
        }
    }

    if (!copy_file_to_file_eof(fin, fout)) {
        return false;
    }

    auto close_ret = fout.close();
    if (!close_ret) {
        LOGE("%s: Failed to close file: %s",
             output_file.c_str(), close_ret.error().message().c_str());
        return false;
    }

    return true;
}

bool InstallerUtil::replace_file(const std::string &replace,
                                 const std::string &with)
{
    struct stat sb;
    int sb_ret;

    sb_ret = stat(replace.c_str(), &sb);
    if (sb_ret < 0 && errno != ENOENT) {
        LOGE("%s: Failed to stat file: %s",
             replace.c_str(), strerror(errno));
        return false;
    }

    if (rename(with.c_str(), replace.c_str()) < 0) {
        LOGE("%s: Failed to rename from %s: %s",
             replace.c_str(), with.c_str(), strerror(errno));
        return false;
    }

    if (sb_ret == 0) {
        if (chown(replace.c_str(), sb.st_uid, sb.st_gid) < 0) {
            LOGE("%s: Failed to chown file: %s",
                 replace.c_str(), strerror(errno));
            return false;
        }

        if (chmod(replace.c_str(), sb.st_mode & 0777) < 0) {
            LOGE("%s: Failed to chmod file: %s",
                 replace.c_str(), strerror(errno));
            return false;
        }
    }

    return true;
}

bool InstallerUtil::copy_file_to_file(File &fin, File &fout, uint64_t to_copy)
{
    char buf[10240];

    while (to_copy > 0) {
        size_t to_read = static_cast<size_t>(
                std::min<uint64_t>(to_copy, sizeof(buf)));

        auto n = file_read_retry(fin, buf, to_read);
        if (!n || n.value() != to_read) {
            LOGE("Failed to read data: %s", n.error().message().c_str());
            return false;
        }

        n = file_write_retry(fout, buf, to_read);
        if (!n || n.value() != to_read) {
            LOGE("Failed to write data: %s", n.error().message().c_str());
            return false;
        }

        to_copy -= to_read;
    }

    return true;
}

bool InstallerUtil::copy_file_to_file_eof(File &fin, File &fout)
{
    char buf[10240];

    while (true) {
        auto n_read = file_read_retry(fin, buf, sizeof(buf));
        if (!n_read) {
            LOGE("Failed to read data: %s", n_read.error().message().c_str());
            return false;
        } else if (n_read.value() == 0) {
            break;
        }

        auto n_written = file_write_retry(fout, buf, n_read.value());
        if (!n_written || n_written.value() != n_read.value()) {
            LOGE("Failed to write data: %s",
                 n_written.error().message().c_str());
            return false;
        }
    }

    return true;
}

}
