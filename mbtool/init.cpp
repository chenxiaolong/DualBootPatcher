/*
 * Copyright (C) 2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "init.h"

#include <memory>

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/mount.h>
#include <sys/poll.h>
#include <unistd.h>

#include "initwrapper/devices.h"
#include "initwrapper/util.h"
#include "mount_fstab.h"
#include "reboot.h"
#include "util/chown.h"
#include "util/finally.h"
#include "util/logging.h"
#include "util/string.h"

namespace mb
{

typedef std::unique_ptr<std::FILE, int (*)(std::FILE *)> file_ptr;
typedef std::unique_ptr<DIR, int (*)(DIR *)> dir_ptr;

static void init_usage(bool error)
{
    FILE *stream = error ? stderr : stdout;

    fprintf(stream,
            "Usage: init"
            "\n"
            "Note: This tool takes no arguments\n"
            "\n"
            "This tool mounts the appropriate multiboot filesystems (as determined\n"
            "by /romid) before exec()'ing the real /init binary. A tmpfs /dev\n"
            "filesystem will be created for device nodes as the kernel is probed\n"
            "for block devices. When the filesystems have been mounted, the device\n"
            "nodes will be removed and /dev unmounted. The real init binary is\n"
            "expected to be located at /init.orig in the ramdisk. mbtool will\n"
            "remove the /init -> /mbtool symlink and rename /init.orig to /init.\n"
            "/init will then be launched with no arguments.\n");
}

static std::string find_fstab()
{
    dir_ptr dir(opendir("/"), closedir);
    if (!dir) {
        return std::string();
    }

    struct dirent *ent;
    while ((ent = readdir(dir.get()))) {
        // Look for *.rc files
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0
                || !util::ends_with(ent->d_name, ".rc")) {
            continue;
        }

        std::string path("/");
        path += ent->d_name;

        file_ptr fp(std::fopen(path.c_str(), "r"), std::fclose);
        if (!fp) {
            continue;
        }

        char *line = nullptr;
        size_t len = 0;
        ssize_t read = 0;

        auto free_line = util::finally([&]{
            free(line);
        });

        while ((read = getline(&line, &len, fp.get())) >= 0) {
            char *ptr = strstr(line, "mount_all");
            if (!ptr) {
                continue;
            }
            ptr += 9;

            // Find the argument to mount_all
            while (isspace(*ptr)) {
                ++ptr;
            }

            // Strip everything after next whitespace
            for (char *p = ptr; *p; ++p) {
                if (isspace(*p)) {
                    *p = '\0';
                    break;
                }
            }

            // Check if we need to strip "/." and ".gen" to remain compatible
            // with boot images patched by an older version of libmbp
            char *slash_ptr = strstr(ptr, "/.");
            if (slash_ptr && util::ends_with(slash_ptr, ".gen")) {
                ptr = slash_ptr + 2;
                ptr[strlen(ptr) - 4] = '\0';
            }

            LOGD("Trying fstab: %s", ptr);

            // Check if fstab exists
            struct stat sb;
            if (stat(ptr, &sb) < 0) {
                LOGE("Failed to stat fstab %s: %s", ptr, strerror(errno));
                continue;
            }

            return ptr;
        }
    }

    return std::string();
}

static bool fix_file_contexts()
{
    file_ptr fp_old(std::fopen("/file_contexts", "rb"), std::fclose);
    if (!fp_old) {
        if (errno == ENOENT) {
            return true;
        } else {
            LOGE("Failed to open /file_contexts: %s", strerror(errno));
            return false;
        }
    }

    file_ptr fp_new(std::fopen("/file_contexts.new", "wb"), std::fclose);
    if (!fp_new) {
        LOGE("Failed to open /file_contexts.new for writing: %s",
             strerror(errno));
        return false;
    }

    char *line = nullptr;
    size_t len = 0;
    ssize_t read = 0;

    auto free_line = util::finally([&]{
        free(line);
    });

    while ((read = getline(&line, &len, fp_old.get())) >= 0) {
        if (util::starts_with(line, "/data/media(")
                && !strstr(line, "<<none>>")) {
            continue;
        }

        if (fwrite(line, 1, read, fp_new.get()) != (std::size_t) read) {
            LOGE("Failed to write to /file_contexts.new: %s", strerror(errno));
            return false;
        }
    }

    static const char *new_contexts =
            "\n"
            "/data/media(/.*)?      <<none>>\n"
            "/raw(/.*)?             <<none>>\n"
            "/data/multiboot(/.*)?  <<none>>\n";
    fputs(new_contexts, fp_new.get());

    struct stat sb;
    if (fstat(fileno(fp_old.get()), &sb) < 0) {
        LOGE("Failed to stat /file_contexts: %s", strerror(errno));
        return false;
    }

    if (rename("/file_contexts.new", "/file_contexts") < 0) {
        LOGE("Failed to rename /file_contexts.new to /file_contexts: %s",
             strerror(errno));
        return false;
    }

    if (fchmod(fileno(fp_new.get()), sb.st_mode & 0777) < 0) {
        LOGE("Failed to chmod /file_contexts: %s",
             strerror(errno));
        return false;
    }

    // Close files
    fp_old.reset();
    fp_new.reset();

    return true;
}

static bool is_completely_whitespace(const char *str)
{
    while (*str) {
        if (!isspace(*str)) {
            return false;
        }
        ++str;
    }
    return true;
}

static bool add_mbtool_services()
{
    file_ptr fp_old(std::fopen("/init.rc", "rb"), std::fclose);
    if (!fp_old) {
        if (errno == ENOENT) {
            return true;
        } else {
            LOGE("Failed to open /init.rc: %s", strerror(errno));
            return false;
        }
    }

    file_ptr fp_new(std::fopen("/init.rc.new", "wb"), std::fclose);
    if (!fp_new) {
        LOGE("Failed to open /init.rc.new for writing: %s",
             strerror(errno));
        return false;
    }

    char *line = nullptr;
    size_t len = 0;
    ssize_t read = 0;

    auto free_line = util::finally([&]{
        free(line);
    });

    bool has_init_multiboot_rc = false;
    bool has_disabled_installd = false;
    bool inside_service = false;

    while ((read = getline(&line, &len, fp_old.get())) >= 0) {
        if (strstr(line, "import /init.multiboot.rc")) {
            has_init_multiboot_rc = true;
        }

        if (util::starts_with(line, "service")) {
            inside_service = strstr(line, "installd") != nullptr;
        } else if (inside_service && is_completely_whitespace(line)) {
            inside_service = false;
        }

        if (inside_service && strstr(line, "disabled")) {
            has_disabled_installd = true;
        }
    }

    rewind(fp_old.get());

    while ((read = getline(&line, &len, fp_old.get())) >= 0) {
        // Load /init.multiboot.rc
        if (!has_init_multiboot_rc && line[0] != '#') {
            has_init_multiboot_rc = true;
            fputs("import /init.multiboot.rc\n", fp_new.get());
        }

        if (fwrite(line, 1, read, fp_new.get()) != (std::size_t) read) {
            LOGE("Failed to write to /init.rc.new: %s", strerror(errno));
            return false;
        }

        // Disable installd. mbtool's appsync will spawn it on demand
        if (!has_disabled_installd
                && util::starts_with(line, "service")
                && strstr(line, "installd")) {
            fputs("    disabled\n", fp_new.get());
        }
    }

    struct stat sb;
    if (fstat(fileno(fp_old.get()), &sb) < 0) {
        LOGE("Failed to stat /init.rc: %s", strerror(errno));
        return false;
    }

    if (rename("/init.rc.new", "/init.rc") < 0) {
        LOGE("Failed to rename /init.rc.new to /init.rc: %s",
             strerror(errno));
        return false;
    }

    if (fchmod(fileno(fp_new.get()), sb.st_mode & 0777) < 0) {
        LOGE("Failed to chmod /init.rc: %s",
             strerror(errno));
        return false;
    }

    // Close files
    fp_old.reset();
    fp_new.reset();

    // Create /init.multiboot.rc
    file_ptr fp_multiboot(std::fopen("/init.multiboot.rc", "wb"), std::fclose);
    if (!fp_multiboot) {
        LOGE("Failed to open /init.multiboot.rc for writing: %s",
             strerror(errno));
        return false;
    }

    static const char *init_multiboot_rc =
            "service mbtooldaemon /mbtool daemon\n"
            "    class main\n"
            "    user root\n"
            "    oneshot\n"
            "\n"
            "service appsync /mbtool appsync\n"
            "    class main\n"
            "    socket installd stream 600 system system\n";

    fputs(init_multiboot_rc, fp_multiboot.get());

    fchmod(fileno(fp_multiboot.get()), 0750);
    fp_multiboot.reset();

    return true;
}

int init_main(int argc, char *argv[])
{
    int opt;

    static struct option long_options[] = {
        {"help",   no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };

    int long_index = 0;

    while ((opt = getopt_long(argc, argv, "h", long_options, &long_index)) != -1) {
        switch (opt) {
        case 'h':
            init_usage(false);
            return EXIT_SUCCESS;

        default:
            init_usage(true);
            return EXIT_FAILURE;
        }
    }

    // There should be no other arguments
    if (argc - optind != 0) {
        init_usage(true);
        return EXIT_FAILURE;
    }

    umask(0);

    mkdir("/dev", 0755);
    mkdir("/proc", 0755);
    mkdir("/sys", 0755);

    mount("tmpfs", "/dev", "tmpfs", MS_NOSUID, "mode=0755");
    mkdir("/dev/pts", 0755);
    mkdir("/dev/socket", 0755);
    mount("devpts", "/dev/pts", "devpts", 0, nullptr);
    mount("proc", "/proc", "proc", 0, nullptr);
    mount("sysfs", "/sys", "sysfs", 0, nullptr);

    open_devnull_stdio();
    util::log_set_logger(std::make_shared<util::KmsgLogger>());

    // Start probing for devices
    device_init();

    std::string fstab = find_fstab();
    if (fstab.empty()) {
        LOGE("Failed to find a suitable fstab file");
        reboot_directly("recovery");
        return EXIT_FAILURE;
    }

    mkdir("/system", 0755);
    mkdir("/cache", 0770);
    mkdir("/data", 0771);
    util::chown("/cache", "system", "cache", 0);
    util::chown("/data", "system", "system", 0);

    // Mount fstab and write new redacted version
    if (!mount_fstab(fstab, true)) {
        LOGE("Failed to mount fstab");
        reboot_directly("recovery");
        return EXIT_FAILURE;
    }

    LOGE("Successfully mounted fstab");

    fix_file_contexts();
    add_mbtool_services();

    // Kill uevent thread and close uevent socket
    device_close();

    // Unmount partitions
    umount("/dev/pts");
    umount("/dev");
    umount("/proc");
    umount("/sys");
    rmdir("/dev");
    rmdir("/proc");
    rmdir("/sys");

    // Start real init
    unlink("/init");
    rename("/init.orig", "/init");

    execlp("/init", "/init", nullptr);
    LOGE("Failed to exec real init: %s", strerror(errno));
    reboot_directly("recovery");
    return EXIT_FAILURE;
}

}
