/*
 * Copyright (C) 2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

// libmbsparse
#include "mbsparse/sparse.h"

// libmbdevice
#include "mbdevice/json.h"
#include "mbdevice/validate.h"

// libmbutil
#include "mbutil/command.h"
#include "mbutil/copy.h"
#include "mbutil/finally.h"
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

static int interface;
static int output_fd;
static const char *zip_file;

static char sales_code[10];
static std::string system_block_dev;
static std::string boot_block_dev;

__attribute__((format(printf, 1, 2)))
void ui_print(const char *fmt, ...)
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

void set_progress(double frac)
{
    dprintf(output_fd, "set_progress %f\n", frac);
}

__attribute__((format(printf, 1, 2)))
void error(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
}

__attribute__((format(printf, 1, 2)))
void info(const char *fmt, ...)
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
    const char *argv[] = { "mount", "/system", nullptr };
    int status = mb::util::run_command(argv[0], argv, nullptr, nullptr,
                                       nullptr, nullptr);
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
    const char *argv[] = { "umount", "/system", nullptr };
    int status = mb::util::run_command(argv[0], argv, nullptr, nullptr,
                                       nullptr, nullptr);
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

static bool la_skip_to(archive *a, const char *filename, archive_entry **entry)
{
    int ret;
    while ((ret = archive_read_next_header(a, entry)) == ARCHIVE_OK) {
        const char *name = archive_entry_pathname(*entry);
        if (!name) {
            error("libarchive: Failed to get filename");
            return false;
        }

        if (strcmp(filename, name) == 0) {
            return true;
        }
    }
    if (ret != ARCHIVE_EOF) {
        error("libarchive: Failed to read header: %s", archive_error_string(a));
        return false;
    }

    error("libarchive: Failed to find %s in zip", filename);
    return false;
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
    system_block_dev[0] = '\0';
    boot_block_dev[0] = '\0';

    Device *device;

    {
        archive *a = archive_read_new();
        if (!a) {
            error("Out of memory");
            return false;
        }

        auto close_archive = mb::util::finally([&]{
            archive_read_free(a);
        });

        if (!la_open_zip(a, zip_file)) {
            return false;
        }

        archive_entry *entry;
        if (!la_skip_to(a, DEVICE_JSON_FILE, &entry)) {
            return false;
        }

        static const size_t max_size = 10240;

        if (archive_entry_size(entry) >= max_size) {
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
        MbDeviceJsonError ret;
        device = mb_device_new_from_json(buf.data(), &ret);
        if (!device) {
            error("Failed to load %s", DEVICE_JSON_FILE);
            return false;
        }
    }

    auto free_device = mb::util::finally([&]{
        mb_device_free(device);
    });

    auto flags = mb_device_validate(device);
    if (flags != 0) {
        error("Device definition file is invalid: %" PRIu64, flags);
        return false;
    }

    auto system_devs = mb_device_system_block_devs(device);
    auto boot_devs = mb_device_boot_block_devs(device);

    struct stat sb;

    if (system_devs) {
        for (auto it = system_devs; *it; ++it) {
            if (stat(*it, &sb) == 0 && S_ISBLK(sb.st_mode)) {
                system_block_dev = *it;
                break;
            }
        }
    }
    if (boot_devs) {
        for (auto it = boot_devs; *it; ++it) {
            if (stat(*it, &sb) == 0 && S_ISBLK(sb.st_mode)) {
                boot_block_dev = *it;
                break;
            }
        }
    }

    if (system_block_dev.empty()) {
        error("%s: No system block device specified", DEVICE_JSON_FILE);
        return false;
    }
    if (boot_block_dev.empty()) {
        error("%s: No boot block device specified", DEVICE_JSON_FILE);
        return false;
    }

    info("System block device: %s", system_block_dev.c_str());
    info("Boot block device: %s", boot_block_dev.c_str());

    return true;
}

static bool cb_zip_read(void *buf, uint64_t size, uint64_t *bytes_read,
                        void *user_data)
{
    archive *a = (archive *) user_data;
    uint64_t total = 0;

    while (size > 0) {
        la_ssize_t n = archive_read_data(a, buf, size);
        if (n < 0) {
            error("libarchive: Failed to read data: %s",
                  archive_error_string(a));
            return false;
        } else if (n == 0) {
            break;
        }

        total += n;
        size -= n;
        buf = (char *) buf + n;
    }

    *bytes_read = total;
    return true;
}

#if DEBUG_SKIP_FLASH_SYSTEM
__attribute__((unused))
#endif
static bool extract_sparse_file(const char *zip_filename,
                                const char *out_filename)
{
    struct SparseCtx *ctx;
    char buf[10240];
    uint64_t n;
    bool sparse_ret;
    int fd;
    uint64_t cur_bytes = 0;
    uint64_t max_bytes = 0;
    uint64_t old_bytes = 0;
    double old_ratio;
    double new_ratio;

    archive *a = archive_read_new();
    if (!a) {
        error("Out of memory");
        goto error;
    }

    if (!la_open_zip(a, zip_file)) {
        goto error_la_allocated;
    }

    archive_entry *entry;
    if (!la_skip_to(a, zip_filename, &entry)) {
        goto error_la_allocated;
    }

    ctx = sparseCtxNew();
    if (!ctx) {
        error("Out of memory");
        goto error_la_allocated;
    }

    if (!sparseOpen(ctx, nullptr, nullptr, &cb_zip_read, nullptr, nullptr, a)) {
        error("Failed to open sparse file");
        goto error_sparse_allocated;
    }

    fd = open64(out_filename,
                O_CREAT | O_TRUNC | O_WRONLY | O_CLOEXEC | O_LARGEFILE, 0600);
    if (fd < 0) {
        error("%s: Failed to open: %s", out_filename, strerror(errno));
        goto error_sparse_allocated;
    }

    sparseSize(ctx, &max_bytes);

    set_progress(0);

    while ((sparse_ret = sparseRead(ctx, buf, sizeof(buf), &n)) && n > 0) {
        // Rate limit: update progress only after difference exceeds 0.1%
        old_ratio = (double) old_bytes / max_bytes;
        new_ratio = (double) cur_bytes / max_bytes;
        if (new_ratio - old_ratio >= 0.001) {
            set_progress(new_ratio);
            old_bytes = cur_bytes;
        }

        char *out_ptr = buf;
        ssize_t nwritten;

        do {
            if ((nwritten = write(fd, out_ptr, n)) < 0) {
                error("%s: Failed to write: %s",
                      out_filename, strerror(errno));
                goto error_fd_opened;
            }

            n -= nwritten;
            out_ptr += nwritten;
            cur_bytes += nwritten;
        } while (n > 0);
    }
    if (!sparse_ret) {
        error("Failed to read sparse file %s", zip_filename);
        goto error_fd_opened;
    }

    close(fd);
    sparseCtxFree(ctx);
    archive_read_free(a);
    return true;

error_fd_opened:
    close(fd);
error_sparse_allocated:
    sparseCtxFree(ctx);
error_la_allocated:
    archive_read_free(a);
error:
    return false;
}

static bool extract_raw_file(const char *zip_filename,
                             const char *out_filename)
{
    char buf[10240];
    la_ssize_t n;
    int fd;
    uint64_t cur_bytes = 0;
    uint64_t max_bytes = 0;
    uint64_t old_bytes = 0;
    double old_ratio;
    double new_ratio;

    archive *a = archive_read_new();
    if (!a) {
        error("Out of memory");
        goto error;
    }

    if (!la_open_zip(a, zip_file)) {
        goto error_la_allocated;
    }

    archive_entry *entry;
    if (!la_skip_to(a, zip_filename, &entry)) {
        goto error_la_allocated;
    }

    max_bytes = archive_entry_size(entry);

    fd = open64(out_filename,
                O_CREAT | O_TRUNC | O_WRONLY | O_CLOEXEC | O_LARGEFILE, 0600);
    if (fd < 0) {
        error("%s: Failed to open: %s", out_filename, strerror(errno));
        goto error_la_allocated;
    }

    set_progress(0);

    while ((n = archive_read_data(a, buf, sizeof(buf))) > 0) {
        // Rate limit: update progress only after difference exceeds 0.1%
        old_ratio = (double) old_bytes / max_bytes;
        new_ratio = (double) cur_bytes / max_bytes;
        if (new_ratio - old_ratio >= 0.001) {
            set_progress(new_ratio);
            old_bytes = cur_bytes;
        }

        char *out_ptr = buf;
        ssize_t nwritten;

        do {
            if ((nwritten = write(fd, out_ptr, n)) < 0) {
                error("%s: Failed to write: %s",
                      out_filename, strerror(errno));
                goto error_fd_opened;
            }

            n -= nwritten;
            out_ptr += nwritten;
        } while (n > 0);
    }
    if (n != 0) {
        error("libarchive: %s: Failed to read %s: %s",
              zip_file, zip_filename, archive_error_string(a));
        goto error_fd_opened;
    }

    close(fd);
    archive_read_free(a);
    return true;

error_fd_opened:
    close(fd);
error_la_allocated:
    archive_read_free(a);
error:
    return false;
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
                                  mb::util::COPY_ATTRIBUTES
                                | mb::util::COPY_XATTRS
                                | mb::util::COPY_EXCLUDE_TOP_LEVEL);
    if (!ret) {
        error("Failed to copy %s to %s: %s",
              source_dir, target_dir, strerror(errno));
    }

    return ret;
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
    archive *matcher;
    archive *in;
    archive *out;

    matcher = archive_match_new();
    if (!matcher) {
        error("libarchive: Out of memory when creating matcher");
        goto error;
    }
    in = archive_read_new();
    if (!in) {
        error("libarchive: Out of memory when creating archive reader");
        goto error_matcher_allocated;
    }
    out = archive_write_disk_new();
    if (!out) {
        error("libarchive: Out of memory when creating disk writer");
        goto error_reader_allocated;
    }

    // Only process system/* paths from zip
    if (archive_match_include_pattern(matcher, "system/*") != ARCHIVE_OK) {
        error("libarchive: Failed to add 'system/*' pattern: %s",
              archive_error_string(matcher));
        goto error_writer_allocated;
    }

    // Set up archive reader parameters
    archive_read_support_format_zip(in);

    // Set up disk writer parameters
    archive_write_disk_set_standard_lookup(out);
    archive_write_disk_set_options(out, ARCHIVE_EXTRACT_TIME
                                      | ARCHIVE_EXTRACT_SECURE_SYMLINKS
                                      | ARCHIVE_EXTRACT_SECURE_NODOTDOT
                                      | ARCHIVE_EXTRACT_OWNER
                                      | ARCHIVE_EXTRACT_PERM
                                      | ARCHIVE_EXTRACT_ACL
                                      | ARCHIVE_EXTRACT_XATTR
                                      | ARCHIVE_EXTRACT_FFLAGS
                                      | ARCHIVE_EXTRACT_MAC_METADATA
                                      | ARCHIVE_EXTRACT_SPARSE);

    if (archive_read_open_filename(in, TEMP_CSC_ZIP_FILE, 10240)
            != ARCHIVE_OK) {
        error("libarchive: %s: Failed to open file: %s",
              zip_file, archive_error_string(in));
        goto error_writer_allocated;
    }

    archive_entry *entry;
    int ret;

    while (true) {
        ret = archive_read_next_header(in, &entry);
        if (ret == ARCHIVE_EOF) {
            break;
        } else if (ret == ARCHIVE_RETRY) {
            info("libarchive: %s: Retrying header read", zip_file);
            continue;
        } else if (ret != ARCHIVE_OK) {
            error("libarchive: %s: Failed to read header: %s",
                  zip_file, archive_error_string(in));
            goto error_writer_allocated;
        }

        const char *path = archive_entry_pathname(entry);
        if (!path || !*path) {
            error("libarchive: %s: Header has null or empty filename",
                  zip_file);
            goto error_writer_allocated;
        }

        // Check pattern matches
        if (archive_match_excluded(matcher, entry)) {
            info("Skipping %s", path);
            continue;
        }

        info("Extracting %s", path);

        // Extract file
        ret = archive_read_extract2(in, entry, out);
        if (ret != ARCHIVE_OK) {
            error("%s: %s", path, archive_error_string(in));
            goto error_writer_allocated;
        }
    }

    if (archive_write_close(out) != ARCHIVE_OK) {
        error("libarchive: Failed to close disk writer: %s",
              archive_error_string(out));
        goto error_writer_allocated;
    }

    archive_write_free(out);
    archive_read_free(in);
    archive_match_free(matcher);
    return true;

error_writer_allocated:
    archive_write_free(out);
error_reader_allocated:
    archive_read_free(in);
error_matcher_allocated:
    archive_match_free(matcher);
error:
    return false;
}

static bool flash_csc()
{
    int status;
    bool unmounted_dir = false;
    bool unmounted_file = false;

    if (!extract_raw_file(CACHE_SPARSE_FILE, TEMP_CACHE_SPARSE_FILE)) {
        goto error;
    }

    if (!extract_raw_file(FUSE_SPARSE_FILE, TEMP_FUSE_SPARSE_FILE)) {
        goto error;
    }

    if (chmod(TEMP_FUSE_SPARSE_FILE, 0700) < 0) {
        error("%s: Failed to chmod: %s",
              TEMP_FUSE_SPARSE_FILE, strerror(errno));
        goto error;
    }

    // Create temporary file for fuse
    close(open(TEMP_CACHE_MOUNT_FILE, O_CREAT | O_WRONLY | O_CLOEXEC, 0600));

    // Mount sparse file with fuse-sparse
    {
        const char *argv[] = {
            TEMP_FUSE_SPARSE_FILE,
            TEMP_CACHE_SPARSE_FILE,
            TEMP_CACHE_MOUNT_FILE,
            nullptr
        };
        status = mb::util::run_command(argv[0], argv, nullptr, nullptr,
                                       nullptr, nullptr);
    }
    if (status < 0) {
        error("Failed to run command: %s", strerror(errno));
        goto error;
    } else if (WIFSIGNALED(status)) {
        error("Command killed by signal: %d", WTERMSIG(status));
        goto error;
    } else if (WEXITSTATUS(status) != 0) {
        error("Command returned non-zero exit status: %d", WEXITSTATUS(status));
        goto error;
    }

    // Create mount directory
    mkdir(TEMP_CACHE_MOUNT_DIR, 0700);

    // Mount image. libmbutil will automatically take care of associating a loop
    // device and mounting it
    if (!mb::util::mount(TEMP_CACHE_MOUNT_FILE, TEMP_CACHE_MOUNT_DIR, "ext4",
                         MS_RDONLY, "")) {
        error("Failed to mount (%s) %s at %s: %s",
              TEMP_CACHE_MOUNT_FILE, "ext4", TEMP_CACHE_MOUNT_DIR,
              strerror(errno));
        goto error_fuse_mounted;
    }

    // Mount system
    if (!mount_system()) {
        error("Failed to mount system");
        goto error_sparse_mounted;
    }

    if (!flash_csc_zip()) {
        error("Failed to flash CSC zip");
        goto error_system_mounted;
    }

    if (!apply_multi_csc()) {
        error("Failed to apply Multi-CSC");
        goto error_system_mounted;
    }

    umount_system();

    // Looping is necessary in Touchwiz, which sometimes fails to unmount the
    // fuse mountpoint on the first try (kernel bug?)
    static constexpr int max_attempts = 5;

    for (int attempt = 1; attempt <= max_attempts; ++attempt) {
        bool success = true;

        info("[Attempt %d/%d] Unmounting %s",
             attempt, max_attempts, TEMP_CACHE_MOUNT_DIR);
        if (!unmounted_dir) {
            unmounted_dir = mb::util::umount(TEMP_CACHE_MOUNT_DIR);
            if (!unmounted_dir) {
                error("WARNING: Failed to unmount %s: %s",
                      TEMP_CACHE_MOUNT_DIR, strerror(errno));
                success = false;
            }
        }

        info("[Attempt %d/%d] Unmounting %s",
             attempt, max_attempts, TEMP_CACHE_MOUNT_FILE);
        if (!unmounted_file) {
            unmounted_file = mb::util::umount(TEMP_CACHE_MOUNT_FILE);
            if (!unmounted_file) {
                error("WARNING: Failed to unmount %s: %s",
                      TEMP_CACHE_MOUNT_FILE, strerror(errno));
                success = false;
            }
        }

        if (success) {
            info("Successfully unmounted temporary cache image mountpoints");
            break;
        } else if (attempt != max_attempts) {
            info("[Attempt %d/%d] Waiting 1 second before next attempt",
                 attempt, max_attempts);
            sleep(1);
        }
    }

    return true;

error_system_mounted:
    umount_system();
error_sparse_mounted:
    mb::util::umount(TEMP_CACHE_MOUNT_DIR);
error_fuse_mounted:
    umount(TEMP_CACHE_MOUNT_FILE);
error:
    return false;
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

    ui_print("------ EXPERIMENTAL ------");
    ui_print("Patched Odin image flasher");
    ui_print("------ EXPERIMENTAL ------");

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

    // Flash system.img.ext4
#if DEBUG_SKIP_FLASH_SYSTEM
    ui_print("[DEBUG] Skipping flashing of system image");
#else
    ui_print("Flashing system image");
    if (!extract_sparse_file(SYSTEM_SPARSE_FILE, system_block_dev.c_str())) {
        ui_print("Failed to flash system image");
        return false;
    }
    ui_print("Successfully flashed system image");
#endif

    // Flash CSC from cache.img.ext4
#if DEBUG_SKIP_FLASH_CSC
    ui_print("[DEBUG] Skipping flashing of CSC");
#else
    ui_print("Flashing CSC from cache image");
    if (!flash_csc()) {
        ui_print("Failed to flash CSC");
        return false;
    }
    ui_print("Successfully flashed CSC");
#endif

    // Flash boot.img
#if DEBUG_SKIP_FLASH_BOOT
    ui_print("[DEBUG] Skipping flashing of boot image");
#else
    ui_print("Flashing boot image");
    if (!extract_raw_file(BOOT_IMAGE_FILE, boot_block_dev.c_str())) {
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

    char *ptr;

    interface = strtol(argv[1], &ptr, 10);
    if (*ptr != '\0' || interface < 0) {
        error("Invalid interface");
        return EXIT_FAILURE;
    }

    output_fd = strtol(argv[2], &ptr, 10);
    if (*ptr != '\0' || output_fd < 0) {
        error("Invalid output fd");
        return EXIT_FAILURE;
    }

    zip_file = argv[3];

    return flash_zip() ? EXIT_SUCCESS : EXIT_FAILURE;
}
