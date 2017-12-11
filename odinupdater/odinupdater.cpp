/*
 * Copyright (C) 2016-2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include <algorithm>
#include <memory>
#include <vector>

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <fcntl.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/wait.h>

// libmbcommon
#include "mbcommon/error_code.h"
#include "mbcommon/file/callbacks.h"
#include "mbcommon/file/standard.h"
#include "mbcommon/finally.h"
#include "mbcommon/integer.h"

// libmbsparse
#include "mbsparse/sparse.h"

// libmbdevice
#include "mbdevice/json.h"

// libmbutil
#include "mbutil/command.h"
#include "mbutil/copy.h"
#include "mbutil/mount.h"
#include "mbutil/properties.h"

// minizip
#include <archive.h>
#include <archive_entry.h>

#define DEBUG_SKIP_FLASH_SYSTEM 0
#define DEBUG_SKIP_FLASH_CSC    0
#define DEBUG_SKIP_FLASH_BOOT   0

#define SYSTEM_SPARSE_FILE      "system.img.sparse"
#define CACHE_SPARSE_FILE       "cache.img.sparse"
#define BOOT_IMAGE_FILE         "boot.img"
#define FUSE_SPARSE_FILE        "fuse-sparse"
#define DEVICE_JSON_FILE        "multiboot/device.json"

#define TEMP_CACHE_SPARSE_FILE  "/tmp/cache.img.ext4"
#define TEMP_CACHE_MOUNT_FILE   "/tmp/cache.img"
#define TEMP_CACHE_MOUNT_DIR    "/tmp/cache"
#define TEMP_CSC_ZIP_FILE       TEMP_CACHE_MOUNT_DIR "/recovery/sec_csc.zip"
#define TEMP_FUSE_SPARSE_FILE   "/tmp/fuse-sparse"

#define EFS_SALES_CODE_FILE     "/efs/imei/mps_code.dat"

#define PROP_SYSTEM_DEV         "system"
#define PROP_BOOT_DEV           "boot"

typedef std::unique_ptr<archive, decltype(archive_free) *> ScopedArchive;

using namespace mb::device;

enum class ExtractResult : uint8_t
{
    Ok,
    Missing,
    Error,
};

static int interface;
static int output_fd;
static const char *zip_file;

static char sales_code[10];
static std::string system_block_dev;
static std::string boot_block_dev;

MB_PRINTF(1, 2)
static void ui_print(const char *fmt, ...)
{
    va_list ap;
    va_list copy;

    va_start(ap, fmt);

    dprintf(output_fd, "ui_print ");
    va_copy(copy, ap);
    vdprintf(output_fd, fmt, ap);
    va_end(copy);
    dprintf(output_fd, "\nui_print\n");

    fputs("[UI] ", stdout);
    va_copy(copy, ap);
    vprintf(fmt, ap);
    va_end(copy);
    fputc('\n', stdout);

    va_end(ap);
}

static void set_progress(double frac)
{
    dprintf(output_fd, "set_progress %f\n", frac);
}

MB_PRINTF(1, 2)
static void error(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
}

MB_PRINTF(1, 2)
static void info(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    fputc('\n', stdout);
}

static bool mount_system()
{
    // mbtool will redirect the call
    std::vector<std::string> argv{ "mount", "/system" };
    int status = mb::util::run_command(argv[0], argv, {}, {}, nullptr, nullptr);
    if (status < 0) {
        error("Failed to run command: %s", strerror(errno));
        return false;
    } else if (WIFSIGNALED(status)) {
        error("Command killed by signal: %d", WTERMSIG(status));
        return false;
    }

    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

static bool umount_system()
{
    // mbtool will redirect the call
    std::vector<std::string> argv{ "umount", "/system" };
    int status = mb::util::run_command(argv[0], argv, {}, {}, nullptr, nullptr);
    if (status < 0) {
        error("Failed to run command: %s", strerror(errno));
        return false;
    } else if (WIFSIGNALED(status)) {
        error("Command killed by signal: %d", WTERMSIG(status));
        return false;
    }

    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

static bool la_open_zip(archive *a, const char *filename)
{
    if (archive_read_support_format_zip(a) != ARCHIVE_OK) {
        error("libarchive: Failed to enable zip support: %s",
              archive_error_string(a));
        return false;
    }

    if (archive_read_open_filename(a, zip_file, 10240) != ARCHIVE_OK) {
        error("libarchive: %s: Failed to open zip: %s",
              filename, archive_error_string(a));
        return false;
    }

    return true;
}

static ExtractResult la_skip_to(archive *a, const char *filename,
                                archive_entry **entry)
{
    int ret;
    while ((ret = archive_read_next_header(a, entry)) == ARCHIVE_OK) {
        const char *name = archive_entry_pathname(*entry);
        if (!name) {
            error("libarchive: Failed to get filename");
            return ExtractResult::Error;
        }

        if (strcmp(filename, name) == 0) {
            return ExtractResult::Ok;
        }
    }
    if (ret != ARCHIVE_EOF) {
        error("libarchive: Failed to read header: %s", archive_error_string(a));
        return ExtractResult::Error;
    }

    error("libarchive: Failed to find %s in zip", filename);
    return ExtractResult::Missing;
}

static bool load_sales_code()
{
    int fd = open64(EFS_SALES_CODE_FILE, O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        error("%s: Failed to open file: %s",
              EFS_SALES_CODE_FILE, strerror(errno));
        return false;
    } else {
        ssize_t n = read(fd, sales_code, sizeof(sales_code) - 1);
        close(fd);
        if (n < 0) {
            error("%s: Failed to read file: %s",
                  EFS_SALES_CODE_FILE, strerror(errno));
            return false;
        }
        sales_code[n] = '\0';

        char *newline = strchr(sales_code, '\n');
        if (newline) {
            *newline = '\0';
        }
    }

    if (!*sales_code) {
        error("Sales code is empty");
        return false;
    }

    info("EFS partition says sales code is: %s", sales_code);
    return true;
}

static bool load_block_devs()
{
    system_block_dev.clear();
    boot_block_dev.clear();

    Device device;

    {
        archive *a = archive_read_new();
        if (!a) {
            error("Out of memory");
            return false;
        }

        auto close_archive = mb::finally([&]{
            archive_read_free(a);
        });

        if (!la_open_zip(a, zip_file)) {
            return false;
        }

        archive_entry *entry;
        if (la_skip_to(a, DEVICE_JSON_FILE, &entry) != ExtractResult::Ok) {
            return false;
        }

        static constexpr size_t max_size = 10240;

        if (archive_entry_size(entry) >= static_cast<int64_t>(max_size)) {
            error("%s is too large", DEVICE_JSON_FILE);
            return false;
        }

        std::vector<char> buf(max_size);
        la_ssize_t n;

        n = archive_read_data(a, buf.data(), buf.size() - 1);
        if (n < 0) {
            error("libarchive: %s: Failed to read %s: %s",
                  zip_file, DEVICE_JSON_FILE, archive_error_string(a));
            return false;
        }

        // Buffer is already NULL-terminated
        JsonError ret;
        if (!device_from_json(buf.data(), device, ret)) {
            error("Failed to load %s", DEVICE_JSON_FILE);
            return false;
        }
    }

    auto flags = device.validate();
    if (flags) {
        error("Device definition file is invalid: %" PRIu64,
              static_cast<uint64_t>(flags));
        return false;
    }

    auto is_blk_device = [](const std::string &path) {
        struct stat sb;
        return stat(path.c_str(), &sb) == 0 && S_ISBLK(sb.st_mode);
    };

#if !DEBUG_SKIP_FLASH_SYSTEM
    {
        auto devs = device.system_block_devs();
        auto it = std::find_if(devs.begin(), devs.end(), is_blk_device);
        if (it == devs.end()) {
            error("%s: No system block device specified", DEVICE_JSON_FILE);
            return false;
        }
        system_block_dev = *it;
    }
#endif

#if !DEBUG_SKIP_FLASH_BOOT
    {
        auto devs = device.boot_block_devs();
        auto it = std::find_if(devs.begin(), devs.end(), is_blk_device);
        if (it == devs.end()) {
            error("%s: No boot block device specified", DEVICE_JSON_FILE);
            return false;
        }
        boot_block_dev = *it;
    }
#endif

    info("System block device: %s", system_block_dev.c_str());
    info("Boot block device: %s", boot_block_dev.c_str());

    return true;
}

static mb::oc::result<size_t> cb_zip_read(mb::File &file, void *userdata,
                                          void *buf, size_t size)
{
    (void) file;

    archive *a = static_cast<archive *>(userdata);
    uint64_t total = 0;

    while (size > 0) {
        la_ssize_t n = archive_read_data(a, buf, size);
        if (n < 0) {
            error("libarchive: Failed to read data: %s",
                  archive_error_string(a));
            return mb::ec_from_errno(archive_errno(a));
        } else if (n == 0) {
            break;
        }

        total += static_cast<size_t>(n);
        size -= static_cast<size_t>(n);
        buf = static_cast<char *>(buf) + n;
    }

    return static_cast<size_t>(total);
}

#if DEBUG_SKIP_FLASH_SYSTEM
MB_UNUSED
#endif
static ExtractResult extract_sparse_file(const char *zip_filename,
                                         const char *out_filename)
{
    ScopedArchive a{archive_read_new(), &archive_read_free};
    mb::CallbackFile file;
    mb::sparse::SparseFile sparse_file;
    mb::StandardFile out_file;

    if (!a) {
        error("Out of memory");
        return ExtractResult::Error;
    }

    if (!la_open_zip(a.get(), zip_file)) {
        return ExtractResult::Error;
    }

    archive_entry *entry;
    auto result = la_skip_to(a.get(), zip_filename, &entry);
    if (result != ExtractResult::Ok) {
        return result;
    }

    auto open_ret = file.open(nullptr, nullptr, &cb_zip_read, nullptr, nullptr,
                              nullptr, a.get());
    if (!open_ret) {
        error("Failed to open sparse file in zip: %s",
              open_ret.error().message().c_str());
        return ExtractResult::Error;
    }

    open_ret = sparse_file.open(&file);
    if (!open_ret) {
        error("Failed to open sparse file: %s",
              open_ret.error().message().c_str());
        return ExtractResult::Error;
    }

    open_ret = out_file.open(out_filename, mb::FileOpenMode::WriteOnly);
    if (!open_ret) {
        error("%s: Failed to open for writing: %s",
              out_filename, open_ret.error().message().c_str());
        return ExtractResult::Error;
    }

    char buf[10240];
    uint64_t cur_bytes = 0;
    uint64_t max_bytes = sparse_file.size();
    uint64_t old_bytes = 0;

    set_progress(0);

    while (true) {
        auto n = sparse_file.read(buf, sizeof(buf));
        if (!n) {
            error("Failed to read sparse file %s: %s",
                  zip_filename, n.error().message().c_str());
            return ExtractResult::Error;
        } else if (n.value() == 0) {
            break;
        }

        // Rate limit: update progress only after difference exceeds 0.1%
        double old_ratio = static_cast<double>(old_bytes) / max_bytes;
        double new_ratio = static_cast<double>(cur_bytes) / max_bytes;
        if (new_ratio - old_ratio >= 0.001) {
            set_progress(new_ratio);
            old_bytes = cur_bytes;
        }

        char *out_ptr = buf;

        while (n.value() > 0) {
            auto n_written = out_file.write(buf, n.value());
            if (!n_written) {
                error("%s: Failed to write file: %s",
                      out_filename, n_written.error().message().c_str());
                return ExtractResult::Error;
            }

            n.value() -= n_written.value();
            out_ptr += n_written.value();
            cur_bytes += n_written.value();
        }
    }

    auto close_ret = out_file.close();
    if (!close_ret) {
        error("%s: Failed to close file: %s",
              out_filename, close_ret.error().message().c_str());
        return ExtractResult::Error;
    }

    return ExtractResult::Ok;
}

static ExtractResult extract_raw_file(const char *zip_filename,
                                      const char *out_filename)
{
    ScopedArchive a{archive_read_new(), &archive_read_free};
    char buf[10240];
    la_ssize_t n;
    int fd;
    uint64_t cur_bytes = 0;
    uint64_t max_bytes = 0;
    uint64_t old_bytes = 0;
    double old_ratio;
    double new_ratio;

    if (!a) {
        error("Out of memory");
        return ExtractResult::Error;
    }

    if (!la_open_zip(a.get(), zip_file)) {
        return ExtractResult::Error;
    }

    archive_entry *entry;
    auto result = la_skip_to(a.get(), zip_filename, &entry);
    if (result != ExtractResult::Ok) {
        return result;
    }

    max_bytes = static_cast<uint64_t>(archive_entry_size(entry));

    fd = open64(out_filename,
                O_CREAT | O_TRUNC | O_WRONLY | O_CLOEXEC | O_LARGEFILE, 0600);
    if (fd < 0) {
        error("%s: Failed to open: %s", out_filename, strerror(errno));
        return ExtractResult::Error;
    }

    auto close_fd = mb::finally([fd]{
        close(fd);
    });

    set_progress(0);

    while ((n = archive_read_data(a.get(), buf, sizeof(buf))) > 0) {
        // Rate limit: update progress only after difference exceeds 0.1%
        old_ratio = static_cast<double>(old_bytes) / max_bytes;
        new_ratio = static_cast<double>(cur_bytes) / max_bytes;
        if (new_ratio - old_ratio >= 0.001) {
            set_progress(new_ratio);
            old_bytes = cur_bytes;
        }

        char *out_ptr = buf;
        ssize_t nwritten;

        do {
            if ((nwritten = write(fd, out_ptr, static_cast<size_t>(n))) < 0) {
                error("%s: Failed to write: %s",
                      out_filename, strerror(errno));
                return ExtractResult::Error;
            }

            n -= nwritten;
            out_ptr += nwritten;
        } while (n > 0);
    }
    if (n != 0) {
        error("libarchive: %s: Failed to read %s: %s",
              zip_file, zip_filename, archive_error_string(a.get()));
        return ExtractResult::Error;
    }

    return ExtractResult::Ok;
}

static bool copy_dir_if_exists(const char *source_dir,
                               const char *target_dir)
{
    struct stat sb;
    if (stat(source_dir, &sb) < 0) {
        if (errno == ENOENT) {
            info("Skipping %s -> %s copy as source doesn't exist",
                 source_dir, target_dir);
            return true;
        } else {
            error("%s: Failed to stat: %s", source_dir, strerror(errno));
            return false;
        }
    }

    if (!S_ISDIR(sb.st_mode)) {
        error("%s: Source path is not a directory", source_dir);
        return false;
    }

    bool ret = mb::util::copy_dir(source_dir, target_dir,
                                  mb::util::CopyFlag::CopyAttributes
                                | mb::util::CopyFlag::CopyXattrs
                                | mb::util::CopyFlag::ExcludeTopLevel);
    if (!ret) {
        error("Failed to copy %s to %s: %s",
              source_dir, target_dir, strerror(errno));
    }

    return ret;
}

// Looping is necessary in Touchwiz, which sometimes fails to unmount the
// fuse mountpoint on the first try (kernel bug?)
static bool retry_unmount(const char *mount_point, unsigned int attempts)
{
    for (unsigned int attempt = 0; attempt < attempts; ++attempt) {
        info("[Attempt %d/%d] Unmounting %s",
             attempt + 1, attempts, mount_point);

        if (!mb::util::umount(TEMP_CACHE_MOUNT_DIR)) {
            error("WARNING: Failed to unmount %s: %s",
                  mount_point, strerror(errno));
            info("[Attempt %d/%d] Waiting 1 second before next attempt",
                 attempt + 1, attempts);
            sleep(1);
            continue;
        }

        info("Successfully unmounted temporary cache image mountpoints");
        return true;
    }

    return false;
}

static bool apply_multi_csc()
{
    info("Applying Multi-CSC");

    static const char *common_system = "/system/csc/common/system";
    char sales_code_system[100];
    char sales_code_csc_contents[100];

    snprintf(sales_code_system, sizeof(sales_code_system),
             "/system/csc/%s/system", sales_code);
    snprintf(sales_code_csc_contents, sizeof(sales_code_csc_contents),
             "/system/csc/%s/csc_contents", sales_code);

    // TODO: TouchWiz hard links files that go in /system/app

    if (!copy_dir_if_exists(common_system, "/system")
            || !copy_dir_if_exists(sales_code_system, "/system")) {
        return false;
    }

    if (remove("/system/csc_contents") < 0 && errno != ENOENT) {
        error("%s: Failed to remove: %s",
              "/system/csc_contents", strerror(errno));
        return false;
    }
    if (symlink(sales_code_csc_contents, "/system/csc_contents") < 0) {
        error("Failed to symlink %s to %s: %s",
              sales_code_csc_contents, "/system/csc_contents", strerror(errno));
        return false;
    }

    info("Successfully applied multi-CSC");

    return true;
}

static bool flash_csc_zip()
{
    ScopedArchive matcher{archive_match_new(), &archive_match_free};
    ScopedArchive in{archive_read_new(), &archive_read_free};
    ScopedArchive out{archive_write_disk_new(), &archive_write_free};

    if (!matcher) {
        error("libarchive: Out of memory when creating matcher");
        return false;
    }
    if (!in) {
        error("libarchive: Out of memory when creating archive reader");
        return false;
    }
    if (!out) {
        error("libarchive: Out of memory when creating disk writer");
        return false;
    }

    // Only process system/* paths from zip
    if (archive_match_include_pattern(matcher.get(), "system/*") != ARCHIVE_OK) {
        error("libarchive: Failed to add 'system/*' pattern: %s",
              archive_error_string(matcher.get()));
        return false;
    }

    // Set up archive reader parameters
    archive_read_support_format_zip(in.get());

    // Set up disk writer parameters
    archive_write_disk_set_standard_lookup(out.get());
    archive_write_disk_set_options(out.get(),
                                   ARCHIVE_EXTRACT_TIME
                                 | ARCHIVE_EXTRACT_SECURE_SYMLINKS
                                 | ARCHIVE_EXTRACT_SECURE_NODOTDOT
                                 | ARCHIVE_EXTRACT_OWNER
                                 | ARCHIVE_EXTRACT_PERM
                                 | ARCHIVE_EXTRACT_ACL
                                 | ARCHIVE_EXTRACT_XATTR
                                 | ARCHIVE_EXTRACT_FFLAGS
                                 | ARCHIVE_EXTRACT_MAC_METADATA
                                 | ARCHIVE_EXTRACT_SPARSE);

    if (archive_read_open_filename(in.get(), TEMP_CSC_ZIP_FILE, 10240)
            != ARCHIVE_OK) {
        error("libarchive: %s: Failed to open file: %s",
              zip_file, archive_error_string(in.get()));
        return false;
    }

    archive_entry *entry;
    int ret;

    while (true) {
        ret = archive_read_next_header(in.get(), &entry);
        if (ret == ARCHIVE_EOF) {
            break;
        } else if (ret == ARCHIVE_RETRY) {
            info("libarchive: %s: Retrying header read", zip_file);
            continue;
        } else if (ret != ARCHIVE_OK) {
            error("libarchive: %s: Failed to read header: %s",
                  zip_file, archive_error_string(in.get()));
            return false;
        }

        const char *path = archive_entry_pathname(entry);
        if (!path || !*path) {
            error("libarchive: %s: Header has null or empty filename",
                  zip_file);
            return false;
        }

        // Check pattern matches
        if (archive_match_excluded(matcher.get(), entry)) {
            info("Skipping %s", path);
            continue;
        }

        info("Extracting %s", path);

        // Extract file
        ret = archive_read_extract2(in.get(), entry, out.get());
        if (ret != ARCHIVE_OK) {
            error("%s: %s", path, archive_error_string(in.get()));
            return false;
        }
    }

    if (archive_write_close(out.get()) != ARCHIVE_OK) {
        error("libarchive: Failed to close disk writer: %s",
              archive_error_string(out.get()));
        return false;
    }

    return true;
}

static ExtractResult flash_csc()
{
    int status;
    ExtractResult result;

    result = extract_raw_file(CACHE_SPARSE_FILE, TEMP_CACHE_SPARSE_FILE);
    if (result != ExtractResult::Ok) {
        return result;
    }

    result = extract_raw_file(FUSE_SPARSE_FILE, TEMP_FUSE_SPARSE_FILE);
    if (result != ExtractResult::Ok) {
        return ExtractResult::Error;
    }

    if (chmod(TEMP_FUSE_SPARSE_FILE, 0700) < 0) {
        error("%s: Failed to chmod: %s",
              TEMP_FUSE_SPARSE_FILE, strerror(errno));
        return ExtractResult::Error;
    }

    // Create temporary file for fuse
    close(open(TEMP_CACHE_MOUNT_FILE, O_CREAT | O_WRONLY | O_CLOEXEC, 0600));

    // Mount sparse file with fuse-sparse
    {
        std::vector<std::string> argv{
            TEMP_FUSE_SPARSE_FILE,
            TEMP_CACHE_SPARSE_FILE,
            TEMP_CACHE_MOUNT_FILE
        };
        status = mb::util::run_command(argv[0], argv, {}, {}, nullptr, nullptr);
    }
    if (status < 0) {
        error("Failed to run command: %s", strerror(errno));
        return ExtractResult::Error;
    } else if (WIFSIGNALED(status)) {
        error("Command killed by signal: %d", WTERMSIG(status));
        return ExtractResult::Error;
    } else if (WEXITSTATUS(status) != 0) {
        error("Command returned non-zero exit status: %d", WEXITSTATUS(status));
        return ExtractResult::Error;
    }

    auto unmount_fuse_file = mb::finally([]{
        retry_unmount(TEMP_CACHE_MOUNT_FILE, 5);
    });

    // Create mount directory
    mkdir(TEMP_CACHE_MOUNT_DIR, 0700);

    // Mount image. libmbutil will automatically take care of associating a loop
    // device and mounting it
    if (!mb::util::mount(TEMP_CACHE_MOUNT_FILE, TEMP_CACHE_MOUNT_DIR, "ext4",
                         MS_RDONLY, "")) {
        error("Failed to mount (%s) %s at %s: %s",
              TEMP_CACHE_MOUNT_FILE, "ext4", TEMP_CACHE_MOUNT_DIR,
              strerror(errno));
        return ExtractResult::Error;
    }

    auto unmount_dir = mb::finally([]{
        retry_unmount(TEMP_CACHE_MOUNT_DIR, 5);
    });

    // Mount system
    if (!mount_system()) {
        error("Failed to mount system");
        return ExtractResult::Error;
    }

    auto unmount_system_dir = mb::finally([]{
        umount_system();
    });

    if (!flash_csc_zip()) {
        error("Failed to flash CSC zip");
        return ExtractResult::Error;
    }

    if (!apply_multi_csc()) {
        error("Failed to apply Multi-CSC");
        return ExtractResult::Error;
    }

    return ExtractResult::Ok;
}

static bool flash_zip()
{
    struct stat sb;
    if (stat("/.chroot", &sb) < 0) {
        error("Must be run in mbtool's chroot");
        return false;
    }

    if (chdir("/") < 0) {
        error("Failed to chdir to /");
        return false;
    }

    ui_print("Patched Odin image flasher");

    // Load sales code from EFS partition
    if (!load_sales_code()) {
        return false;
    }

    // Load block device info
    if (!load_block_devs()) {
        return false;
    }

    // Unmount system
    if (mb::util::is_mounted("/system") && !umount_system()) {
        ui_print("Failed to unmount /system");
        return false;
    }

#if !DEBUG_SKIP_FLASH_SYSTEM || !DEBUG_SKIP_FLASH_CSC || !DEBUG_SKIP_FLASH_BOOT
    ExtractResult result;
#endif

    // Flash system.img.ext4
#if DEBUG_SKIP_FLASH_SYSTEM
    ui_print("[DEBUG] Skipping flashing of system image");
#else
    ui_print("Flashing system image");
    result = extract_sparse_file(SYSTEM_SPARSE_FILE, system_block_dev.c_str());
    switch (result) {
    case ExtractResult::Error:
        ui_print("Failed to flash system image");
        return false;
    case ExtractResult::Missing:
        ui_print("[WARNING] System image not found");
        break;
    case ExtractResult::Ok:
        ui_print("Successfully flashed system image");
        break;
    }
#endif

    // Flash CSC from cache.img.ext4
#if DEBUG_SKIP_FLASH_CSC
    ui_print("[DEBUG] Skipping flashing of CSC");
#else
    ui_print("Flashing CSC from cache image");
    result = flash_csc();
    switch (result) {
    case ExtractResult::Error:
        ui_print("Failed to flash CSC");
        return false;
    case ExtractResult::Missing:
        ui_print("[WARNING] Cache image not found. Won't flash CSC");
        break;
    case ExtractResult::Ok:
        ui_print("Successfully flashed CSC");
        break;
    }
#endif

    // Flash boot.img
#if DEBUG_SKIP_FLASH_BOOT
    ui_print("[DEBUG] Skipping flashing of boot image");
#else
    ui_print("Flashing boot image");
    result = extract_raw_file(BOOT_IMAGE_FILE, boot_block_dev.c_str());
    if (result != ExtractResult::Ok) {
        ui_print("Failed to flash boot image");
        return false;
    }
    ui_print("Successfully flashed boot image");
#endif

    ui_print("---");
    ui_print("Flashing completed. The bootloader");
    ui_print("and non-system partitions were left");
    ui_print("untouched.");

    return true;
}

int main(int argc, char *argv[])
{
    if (argc != 4) {
        error("Usage: %s <interface> <fd> <zip file>", argv[0]);
        return EXIT_FAILURE;
    }

    if (!mb::str_to_num(argv[1], 10, interface)) {
        error("Invalid interface: '%s'", argv[1]);
        return EXIT_FAILURE;
    }

    if (!mb::str_to_num(argv[2], 10, output_fd)) {
        error("Invalid output fd: '%s'", argv[2]);
        return EXIT_FAILURE;
    }

    zip_file = argv[3];

    return flash_zip() ? EXIT_SUCCESS : EXIT_FAILURE;
}
