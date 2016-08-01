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

#include <string>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <sys/mount.h>
#include <sys/wait.h>
#include <unistd.h>

#include "mblog/logging.h"
#include "mblog/stdio_logger.h"
#include "mbp/bootimage.h"
#include "mbutil/autoclose/archive.h"
#include "mbutil/copy.h"
#include "mbutil/delete.h"
#include "mbutil/mount.h"

#ifndef TCSASOFT
#define TCSASOFT 0
#endif

#define CHROOT_PATH                     "/cryptfs_tmp"


static int log_mkdir(const char *pathname, mode_t mode)
{
    int ret = mkdir(pathname, mode);
    if (ret < 0) {
        LOGE("%s: Failed to create directory: %s", pathname, strerror(errno));
    }
    return ret;
}

static int log_mount(const char *source, const char *target, const char *fstype,
                     long unsigned int mountflags, const void *data)
{
    int ret = mount(source, target, fstype, mountflags, data);
    if (ret < 0) {
        LOGE("%s: Failed to mount %s (%s): %s",
             target, source, fstype, strerror(errno));
    }
    return ret;
}

static int log_umount(const char *target)
{
    int ret = umount(target);
    if (ret < 0) {
        LOGE("%s: Failed to unmount path: %s", target, strerror(errno));
    }
    return ret;
}

static bool log_unmount_all(const std::string &dir)
{
    bool ret = mb::util::unmount_all(dir);
    if (!ret) {
        LOGE("%s: Failed to unmount all contained mountpoints", dir.c_str());
    }
    return ret;
}

static bool log_delete_recursive(const std::string &path)
{
    bool ret = mb::util::delete_recursive(path);
    if (!ret) {
        LOGE("%s: Failed to recursively delete", path.c_str());
    }
    return ret;
}

static bool extract_recovery(const char *target, const char *recovery_path)
{
    mbp::BootImage bi;
    if (!bi.loadFile(recovery_path)) {
        LOGE("%s: Failed to load recovery image", recovery_path);
        return false;
    }

    const unsigned char *ramdisk_data;
    std::size_t ramdisk_size;
    bi.ramdiskImageC(&ramdisk_data, &ramdisk_size);

    mb::autoclose::archive in(archive_read_new(), archive_read_free);
    mb::autoclose::archive out(archive_write_disk_new(), archive_write_free);

    if (!in || !out) {
        LOGE("Out of memory");
        return false;
    }

    // Set up input
    archive_read_support_filter_gzip(in.get());
    archive_read_support_filter_lzop(in.get());
    archive_read_support_filter_lz4(in.get());
    archive_read_support_filter_lzma(in.get());
    archive_read_support_filter_xz(in.get());
    archive_read_support_format_cpio(in.get());

    int ret = archive_read_open_memory(in.get(),
            const_cast<unsigned char *>(ramdisk_data), ramdisk_size);
    if (ret != ARCHIVE_OK) {
        LOGW("%s: Failed to open recovery ramdisk: %s",
             recovery_path, archive_error_string(in.get()));
        return false;
    }

    // Set up output
    archive_write_disk_set_standard_lookup(out.get());
    archive_write_disk_set_options(out.get(),
                                   ARCHIVE_EXTRACT_ACL |
                                   ARCHIVE_EXTRACT_FFLAGS |
                                   ARCHIVE_EXTRACT_PERM |
                                   ARCHIVE_EXTRACT_SECURE_NODOTDOT |
                                   ARCHIVE_EXTRACT_SECURE_SYMLINKS |
                                   ARCHIVE_EXTRACT_TIME |
                                   ARCHIVE_EXTRACT_UNLINK |
                                   ARCHIVE_EXTRACT_XATTR);

    archive_entry *entry;
    std::string target_path;

    while (true) {
        ret = archive_read_next_header(in.get(), &entry);
        if (ret == ARCHIVE_EOF) {
            break;
        } else if (ret == ARCHIVE_RETRY) {
            LOGW("%s: Retrying header read", recovery_path);
            continue;
        } else if (ret != ARCHIVE_OK) {
            LOGE("%s: Failed to read header: %s",
                 recovery_path, archive_error_string(in.get()));
            return false;
        }

        const char *path = archive_entry_pathname(entry);
        if (!path || !*path) {
            LOGE("%s: Header has null or empty filename", path);
            return false;
        }

        // Build path
        target_path = target;
        if (target_path.back() != '/' && *path != '/') {
            target_path += '/';
        }
        target_path += path;

        archive_entry_set_pathname(entry, target_path.c_str());

        // Extract file
        ret = archive_read_extract2(in.get(), entry, out.get());
        if (ret != ARCHIVE_OK) {
            LOGE("%s: %s", archive_entry_pathname(entry),
                 archive_error_string(in.get()));
            return false;
        }
    }

    if (archive_read_close(in.get()) != ARCHIVE_OK) {
        LOGE("%s: %s", recovery_path, archive_error_string(in.get()));
        return false;
    }

    return true;
}

static bool create_chroot(const char *target, const char *recovery_path)
{
    LOGV("Creating chroot...");

    // Unmount everything previously mounted in the chroot
    if (!log_unmount_all(target)) {
        return false;
    }

    // Remove the chroot directory if it exists
    if (!log_delete_recursive(target)) {
        return false;
    }

    std::string target_dev(target); target_dev += "/dev";
    std::string target_proc(target); target_proc += "/proc";
    std::string target_sys(target); target_sys += "/sys";
    std::string target_data(target); target_data += "/data";
    std::string target_cache(target); target_cache += "/cache";
    std::string target_system(target); target_system += "/system";

    // Set up directories
    if (log_mkdir(target, 0700) < 0
            || log_mkdir(target_dev.c_str(), 0755) < 0
            || log_mkdir(target_proc.c_str(), 0755) < 0
            || log_mkdir(target_sys.c_str(), 0755) < 0
            || log_mkdir(target_data.c_str(), 0755) < 0
            || log_mkdir(target_cache.c_str(), 0755) < 0
            || log_mkdir(target_system.c_str(), 0755) < 0) {
        return false;
    }

    // Mount points
    if (log_mount("/dev", target_dev.c_str(), "", MS_BIND, "") < 0
            || log_mount("/proc", target_proc.c_str(), "", MS_BIND, "") < 0
            || log_mount("/sys", target_sys.c_str(), "", MS_BIND, "") < 0
            || log_mount("tmpfs", target_data.c_str(), "tmpfs", 0, "") < 0
            || log_mount("tmpfs", target_cache.c_str(), "tmpfs", 0, "") < 0
            || log_mount("tmpfs", target_system.c_str(), "tmpfs", 0, "") < 0) {
        return false;
    }

    LOGV("Extracting recovery...");

    if (!extract_recovery(target, recovery_path)) {
        return false;
    }

    return true;
}

static bool destroy_chroot(const char *target)
{
    LOGV("Destroying chroot...");

    std::string target_dev(target); target_dev += "/dev";
    std::string target_proc(target); target_proc += "/proc";
    std::string target_sys(target); target_sys += "/sys";
    std::string target_data(target); target_data += "/data";
    std::string target_cache(target); target_cache += "/cache";
    std::string target_system(target); target_system += "/system";

    log_umount(target_system.c_str());
    log_umount(target_cache.c_str());
    log_umount(target_data.c_str());
    log_umount(target_sys.c_str());
    log_umount(target_proc.c_str());
    log_umount(target_dev.c_str());

    if (!log_unmount_all(target)) {
        return false;
    }

    if (!log_delete_recursive(target)) {
        return false;
    }

    return true;
}

static int execute_tool(const char *chroot_dir, const char *tool_path,
                        int argc, char *argv[])
{
    pid_t pid = fork();
    if (pid < 0) {
        LOGE("Failed to fork: %s", strerror(errno));
        return -1;
    } else if (pid == 0) {
        std::string target(chroot_dir);
        target += "/cryptfstool_rec";

        mb::util::copy_file(tool_path, target, 0);
        chmod(target.c_str(), 0500);

        if (chdir(chroot_dir) < 0) {
            LOGE("%s: Failed to chdir: %s", chroot_dir, strerror(errno));
            _exit(EXIT_FAILURE);
        }
        if (chroot(chroot_dir) < 0) {
            LOGE("%s: Failed to chroot: %s", chroot_dir, strerror(errno));
            _exit(EXIT_FAILURE);
        }

        unsetenv("CRYPTFS_RECOVERY_PATH");
        unsetenv("CRYPTFS_TOOL_PATH");

        if (setenv("LD_LIBRARY_PATH", "/sbin", 1) < 0) {
            LOGE("Failed to set LD_LIBRARY_PATH: %s", strerror(errno));
            _exit(EXIT_FAILURE);
        }

        execv("/cryptfstool_rec", argv);
        LOGE("%s: Failed to exec: %s", tool_path, strerror(errno));
        _exit(127);
    } else {
        // Clear potentially sensitive arguments
        for (int i = 1; i < argc; ++i) {
            memset(argv[i], 0, strlen(argv[i]));
        }

        int status;
        do {
            if (waitpid(pid, &status, 0) < 0) {
                LOGE("Failed to waitpid(): %s", strerror(errno));
                break;
            }
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));

        if (WIFEXITED(status)) {
            mb::log::log(WEXITSTATUS(status) == 0
                         ? mb::log::LogLevel::Verbose
                         : mb::log::LogLevel::Error,
                         "Process completed with status: %d",
                         WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            LOGE("Process killed by signal: %d", WTERMSIG(status));
        }

        return status;
    }
}

int main(int argc, char *argv[])
{
    const char *recovery_path = getenv("CRYPTFS_RECOVERY_PATH");
    const char *tool_path = getenv("CRYPTFS_TOOL_PATH");

    if (!recovery_path || !tool_path) {
        fprintf(stderr, "CRYPTFS_RECOVERY_PATH and CRYPTFS_TOOL_PATH must be set\n");
        return EXIT_FAILURE;
    }

    // Set up logging
    mb::log::log_set_logger(
            std::make_shared<mb::log::StdioLogger>(stderr, false));

    // Make rootfs writable
    if (unshare(CLONE_NEWNS) < 0) {
        LOGE("Failed to unshare mount namespace: %s", strerror(errno));
        return EXIT_FAILURE;
    }
    if (mount("", "/", "", MS_REMOUNT, "") < 0) {
        LOGE("Failed to remount / as writable: %s", strerror(errno));
        return EXIT_FAILURE;
    }

    bool ret = true;

    if (!create_chroot(CHROOT_PATH, recovery_path)) {
        LOGE("Failed to create chroot");
        ret = false;
    }

    if (ret) {
        int status = execute_tool(CHROOT_PATH, tool_path, argc, argv);
        ret = (status == 0 || !WIFEXITED(status)) ? 127 : WEXITSTATUS(status);
    }

    if (!destroy_chroot(CHROOT_PATH)) {
        LOGE("Failed to destroy chroot");
        ret = false;
    }

    return ret ? EXIT_SUCCESS : EXIT_FAILURE;
}
