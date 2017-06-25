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

#include "installer_util.h"

#include <memory>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <sys/stat.h>

#include <archive.h>
#include <archive_entry.h>

#include "mbbootimg/entry.h"
#include "mbbootimg/header.h"
#include "mbbootimg/reader.h"
#include "mbbootimg/writer.h"

#include "mbcommon/file.h"
#include "mbcommon/file_util.h"
#include "mbcommon/file/filename.h"
#include "mbcommon/string.h"

#include "mblog/logging.h"

#include "mbutil/delete.h"
#include "mbutil/finally.h"
#include "mbutil/path.h"

#include "bootimg_util.h"
#include "multiboot.h"

typedef std::unique_ptr<archive, decltype(archive_free) *> ScopedArchive;
typedef std::unique_ptr<archive_entry, decltype(archive_entry_free) *> ScopedArchiveEntry;
typedef std::unique_ptr<FILE, decltype(fclose) *> ScopedFILE;
typedef std::unique_ptr<MbFile, decltype(mb_file_free) *> ScopedMbFile;
typedef std::unique_ptr<MbBiReader, decltype(mb_bi_reader_free) *> ScopedReader;
typedef std::unique_ptr<MbBiWriter, decltype(mb_bi_writer_free) *> ScopedWriter;

namespace mb
{

bool InstallerUtil::unpack_ramdisk(const std::string &input_file,
                                   const std::string &output_dir,
                                   int &format_out,
                                   std::vector<int> &filters_out)
{
    ScopedArchive ain(archive_read_new(), archive_read_free);
    ScopedArchive aout(archive_write_disk_new(), archive_write_free);
    archive_entry *entry;
    std::string target_path;
    int ret;

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

    while (true) {
        ret = archive_read_next_header(ain.get(), &entry);
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

        // Build path
        target_path = output_dir;
        if (!target_path.empty() && target_path.back() != '/' && *path != '/') {
            target_path += '/';
        }
        target_path += path;

        archive_entry_set_pathname(entry, target_path.c_str());

        // Extract file
        ret = archive_read_extract2(ain.get(), entry, aout.get());
        if (ret != ARCHIVE_OK) {
            LOGE("%s: %s", archive_entry_pathname(entry),
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
            std::string relpath;
            if (!util::relative_path(curpath, input_dir, &relpath)) {
                LOGE("Failed to compute relative path of %s starting at %s: %s",
                     curpath, input_dir.c_str(), strerror(errno));
                return false;
            }
            if (relpath.empty()) {
                // If the relative path is empty, then the current path is
                // the root of the directory tree. We don't need that, so
                // skip it.
                continue;
            }
            archive_entry_set_pathname(entry.get(), relpath.c_str());
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
                if (archive_write_data(aout.get(), buf, n) != n) {
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
                                     std::vector<std::function<RamdiskPatcherFn>> &rps)
{
    char *tmpdir = mb_format("%s.XXXXXX", output_file.c_str());
    if (!tmpdir) {
        LOGE("Out of memory");
        return false;
    }

    auto free_temp_dir = util::finally([&]{
        free(tmpdir);
    });

    if (!mkdtemp(tmpdir)) {
        LOGE("Failed to create temporary directory: %s", strerror(errno));
        return false;
    }

    auto delete_temp_dir = util::finally([&]{
        util::delete_recursive(tmpdir);
    });

    ScopedReader bir(mb_bi_reader_new(), &mb_bi_reader_free);
    ScopedWriter biw(mb_bi_writer_new(), &mb_bi_writer_free);
    MbBiHeader *header;
    MbBiEntry *in_entry;
    MbBiEntry *out_entry;
    int ret;

    if (!bir || !biw) {
        LOGE("Failed to allocate reader or writer instance");
        return false;
    }

    // Open input boot image
    ret = mb_bi_reader_enable_format_all(bir.get());
    if (ret != MB_BI_OK) {
        LOGE("Failed to enable input boot image formats: %s",
             mb_bi_reader_error_string(bir.get()));
        return false;
    }
    ret = mb_bi_reader_open_filename(bir.get(), input_file.c_str());
    if (ret != MB_BI_OK) {
        LOGE("%s: Failed to open boot image for reading: %s",
             input_file.c_str(), mb_bi_reader_error_string(bir.get()));
        return false;
    }

    // Open output boot image
    ret = mb_bi_writer_set_format_by_code(
            biw.get(), mb_bi_reader_format_code(bir.get()));
    if (ret != MB_BI_OK) {
        LOGE("Failed to set output boot image format: %s",
             mb_bi_writer_error_string(biw.get()));
        return false;
    }
    ret = mb_bi_writer_open_filename(biw.get(), output_file.c_str());
    if (ret != MB_BI_OK) {
        LOGE("%s: Failed to open boot image for writing: %s",
             output_file.c_str(), mb_bi_writer_error_string(biw.get()));
        return false;
    }

    // Debug
    LOGD("Patching boot image");
    LOGD("- Input: %s", input_file.c_str());
    LOGD("- Output: %s", output_file.c_str());
    LOGD("- Format: %s", mb_bi_reader_format_name(bir.get()));

    // Copy header
    ret = mb_bi_reader_read_header(bir.get(), &header);
    if (ret != MB_BI_OK) {
        LOGE("%s: Failed to read header: %s",
             input_file.c_str(), mb_bi_reader_error_string(bir.get()));
        return false;
    }
    ret = mb_bi_writer_write_header(biw.get(), header);
    if (ret != MB_BI_OK) {
        LOGE("%s: Failed to write header: %s",
             output_file.c_str(), mb_bi_writer_error_string(biw.get()));
        return false;
    }

    // Write entries
    while ((ret = mb_bi_writer_get_entry(biw.get(), &out_entry)) == MB_BI_OK) {
        int type = mb_bi_entry_type(out_entry);

        // Write entry metadata
        ret = mb_bi_writer_write_entry(biw.get(), out_entry);
        if (ret != MB_BI_OK) {
            LOGE("%s: Failed to write entry: %s",
                 output_file.c_str(), mb_bi_writer_error_string(biw.get()));
            return false;
        }

        // Special case for loki aboot
        if (type == MB_BI_ENTRY_ABOOT) {
            if (bi_copy_file_to_data(ABOOT_PARTITION, biw.get())) {
                return false;
            }
        } else {
            ret = mb_bi_reader_go_to_entry(bir.get(), &in_entry, type);
            if (ret == MB_BI_EOF) {
                LOGV("Skipping non existent boot image entry: %d", type);
                continue;
            } else if (ret != MB_BI_OK) {
                LOGE("%s: Failed to go to entry: %d: %s",
                     input_file.c_str(), type,
                     mb_bi_reader_error_string(bir.get()));
                return false;
            }

            if (type == MB_BI_ENTRY_RAMDISK) {
                std::string ramdisk_in(tmpdir);
                ramdisk_in += "/ramdisk.in";
                std::string ramdisk_out(tmpdir);
                ramdisk_out += "/ramdisk.out";

                auto delete_temp_files = util::finally([&]{
                    unlink(ramdisk_in.c_str());
                    unlink(ramdisk_out.c_str());
                });

                if (!bi_copy_data_to_file(bir.get(), ramdisk_in)) {
                    return false;
                }

                if (!patch_ramdisk(ramdisk_in, ramdisk_out, 0, rps)) {
                    return false;
                }

                if (!bi_copy_file_to_data(ramdisk_out, biw.get())) {
                    return false;
                }
            } else if (type == MB_BI_ENTRY_KERNEL) {
                std::string kernel_in(tmpdir);
                kernel_in += "/kernel.in";
                std::string kernel_out(tmpdir);
                kernel_out += "/kernel.out";

                auto delete_temp_files = util::finally([&]{
                    unlink(kernel_in.c_str());
                    unlink(kernel_out.c_str());
                });

                if (!bi_copy_data_to_file(bir.get(), kernel_in)) {
                    return false;
                }

                if (!patch_kernel_rkp(kernel_in, kernel_out)) {
                    return false;
                }

                if (!bi_copy_file_to_data(kernel_out, biw.get())) {
                    return false;
                }
            } else {
                // Copy entry directly
                if (!bi_copy_data_to_data(bir.get(), biw.get())) {
                    return false;
                }
            }
        }
    }

    if (mb_bi_writer_close(biw.get()) != MB_BI_OK) {
        LOGE("%s: Failed to close boot image: %s",
             output_file.c_str(), mb_bi_writer_error_string(biw.get()));
        return false;
    }

    return true;
}

bool InstallerUtil::patch_ramdisk(const std::string &input_file,
                                  const std::string &output_file,
                                  unsigned int depth,
                                  std::vector<std::function<RamdiskPatcherFn>> &rps)
{
    if (depth > 1) {
        LOGV("Ignoring doubly-nested ramdisk");
        return true;
    }

    char *tmpdir = mb_format("%s.XXXXXX", output_file.c_str());
    if (!tmpdir) {
        LOGE("Out of memory");
        return false;
    }

    auto free_temp_dir = util::finally([&]{
        free(tmpdir);
    });

    if (!mkdtemp(tmpdir)) {
        LOGE("Failed to create temporary directory: %s", strerror(errno));
        return false;
    }

    auto delete_temp_dir = util::finally([&]{
        util::delete_recursive(tmpdir);
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
                                      std::vector<std::function<RamdiskPatcherFn>> &rps)
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

    ScopedMbFile fin(mb_file_new(), &mb_file_free);
    ScopedMbFile fout(mb_file_new(), &mb_file_free);
    // TODO: Replace with std::optional after switching to C++17
    std::pair<bool, uint64_t> offset;
    int ret;

    if (!fin || !fout) {
        LOGE("Failed to allocate input or output MbFile handle");
        return false;
    }

    // Open input file
    if (mb_file_open_filename(fin.get(), input_file.c_str(),
                              MB_FILE_OPEN_READ_ONLY) != MB_FILE_OK) {
        LOGE("%s: Failed to open for reading: %s",
             input_file.c_str(), mb_file_error_string(fin.get()));
        return false;
    }

    // Open output file
    if (mb_file_open_filename(fout.get(), output_file.c_str(),
                              MB_FILE_OPEN_WRITE_ONLY) != MB_FILE_OK) {
        LOGE("%s: Failed to open for writing: %s",
             output_file.c_str(), mb_file_error_string(fout.get()));
        return false;
    }

    // Replace pattern
    auto result_cb = [](MbFile *file, void *userdata, uint64_t offset) -> int {
        (void) file;
        std::pair<bool, uint64_t> *ptr =
                static_cast<std::pair<bool, uint64_t> *>(userdata);
        ptr->first = true;
        ptr->second = offset;
        return MB_FILE_OK;
    };

    ret = mb_file_search(fin.get(), -1, -1, 0, source_pattern,
                         sizeof(source_pattern), 1, result_cb, &offset);
    if (ret < 0) {
        LOGE("%s: Error when searching for pattern: %s",
             input_file.c_str(), mb_file_error_string(fin.get()));
        return false;
    }

    // Copy data
    ret = mb_file_seek(fin.get(), 0, SEEK_SET, nullptr);
    if (ret != MB_FILE_OK) {
        LOGE("%s: Failed to seek to beginning: %s",
             input_file.c_str(), mb_file_error_string(fin.get()));
        return false;
    }

    if (offset.first) {
        LOGD("RKP pattern found at offset: 0x%" PRIx64, offset.second);

        if (!copy_file_to_file(fin.get(), fout.get(), offset.second)) {
            return false;
        }

        ret = mb_file_seek(fin.get(), sizeof(source_pattern), SEEK_CUR,
                           nullptr);
        if (ret != MB_FILE_OK) {
            LOGE("%s: Failed to skip pattern: %s",
                 input_file.c_str(), mb_file_error_string(fin.get()));
            return false;
        }

        size_t n;
        ret = mb_file_write_fully(fout.get(), target_pattern,
                                  sizeof(target_pattern), &n);
        if (ret != MB_FILE_OK || n != sizeof(target_pattern)) {
            LOGE("%s: Failed to write target pattern: %s",
                 output_file.c_str(), mb_file_error_string(fout.get()));
            return false;
        }
    }

    if (!copy_file_to_file_eof(fin.get(), fout.get())) {
        return false;
    }

    ret = mb_file_close(fout.get());
    if (ret != MB_FILE_OK) {
        LOGE("%s: Failed to close file: %s",
             output_file.c_str(), mb_file_error_string(fout.get()));
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

bool InstallerUtil::copy_file_to_file(MbFile *fin, MbFile *fout,
                                      uint64_t to_copy)
{
    char buf[10240];
    size_t n;
    int ret;

    while (to_copy > 0) {
        size_t to_read = std::min<uint64_t>(to_copy, sizeof(buf));

        ret = mb_file_read_fully(fin, buf, to_read, &n);
        if (ret != MB_FILE_OK || n != to_read) {
            LOGE("Failed to read data: %s", mb_file_error_string(fin));
            return false;
        }

        ret = mb_file_write_fully(fout, buf, to_read, &n);
        if (ret != MB_FILE_OK || n != to_read) {
            LOGE("Failed to write data: %s", mb_file_error_string(fout));
            return false;
        }

        to_copy -= to_read;
    }

    return true;
}

bool InstallerUtil::copy_file_to_file_eof(MbFile *fin, MbFile *fout)
{
    char buf[10240];
    size_t n_read;
    size_t n_written;
    int ret;

    while (true) {
        ret = mb_file_read_fully(fin, buf, sizeof(buf), &n_read);
        if (ret != MB_FILE_OK) {
            LOGE("Failed to read data: %s", mb_file_error_string(fin));
            return false;
        } else if (n_read == 0) {
            break;
        }

        ret = mb_file_write_fully(fout, buf, n_read, &n_written);
        if (ret != MB_FILE_OK || n_written != n_read) {
            LOGE("Failed to write data: %s", mb_file_error_string(fout));
            return false;
        }
    }

    return true;
}

}
