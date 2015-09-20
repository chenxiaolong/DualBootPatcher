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
#include <unordered_set>

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <sys/klog.h>
#include <sys/mount.h>
#include <sys/poll.h>
#include <unistd.h>

#include "initwrapper/devices.h"
#include "initwrapper/util.h"
#include "mount_fstab.h"
#include "reboot.h"
#include "sepolpatch.h"
#include "util/chown.h"
#include "util/directory.h"
#include "util/finally.h"
#include "util/logging.h"
#include "util/mount.h"
#include "util/selinux.h"
#include "util/string.h"

namespace mb
{

static const char *data_block_devs[] = {
    "/dev/block/platform/msm_sdcc.1/by-name/userdata",
    "/dev/block/platform/f9824900.sdhci/by-name/userdata",
    "/dev/block/platform/15570000.ufs/by-name/USERDATA",
    "/dev/block/platform/15540000.dwmmc0/by-name/USERDATA",
    "/dev/block/platform/dw_mmc.0/by-name/USERDATA",
    "/dev/block/platform/mtk-msdc.0/by-name/userdata",
    nullptr
};

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

            if (strstr(ptr, "goldfish")) {
                LOGV("Skipping goldfish fstab file: %s", ptr);
                continue;
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

// Operating on paths instead of fd's should be safe enough since, at this
// point, we're the only process alive on the system.
static bool replace_file(const char *replace, const char *with)
{
    struct stat sb;
    if (stat(replace, &sb) < 0) {
        LOGE("Failed to stat %s: %s", replace, strerror(errno));
        return false;
    }

    if (rename(with, replace) < 0) {
        LOGE("Failed to rename %s to %s: %s", with, replace, strerror(errno));
        return false;
    }

    if (chmod(replace, sb.st_mode & 0777) < 0) {
        LOGE("Failed to chmod %s: %s", replace, strerror(errno));
        return false;
    }

    return true;
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
            "/data/media              <<none>>\n"
            "/data/media/[0-9]+(/.*)? <<none>>\n"
            "/raw(/.*)?               <<none>>\n"
            "/data/multiboot(/.*)?    <<none>>\n"
            "/cache/multiboot(/.*)?   <<none>>\n"
            "/system/multiboot(/.*)?  <<none>>\n";
    fputs(new_contexts, fp_new.get());

    return replace_file("/file_contexts", "/file_contexts.new");
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

    if (!replace_file("/init.rc", "/init.rc.new")) {
        return false;
    }

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

    return true;
}

static bool strip_manual_mounts()
{
    dir_ptr dir(opendir("/"), closedir);
    if (!dir) {
        return true;
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
            LOGE("Failed to open %s for reading: %s",
                 path.c_str(), strerror(errno));
            continue;
        }

        char *line = nullptr;
        size_t len = 0;
        ssize_t read = 0;

        auto free_line = util::finally([&]{
            free(line);
        });

        std::size_t count = 0;
        std::unordered_set<std::size_t> comment_out;

        // Find out which lines need to be commented out
        while ((read = getline(&line, &len, fp.get())) >= 0) {
            if (strstr(line, "mount")
                    && (strstr(line, "/system")
                    || strstr(line, "/cache")
                    || strstr(line, "/data"))) {
                std::vector<std::string> tokens = util::tokenize(line, " \t\n");
                if (tokens.size() >= 4 && tokens[0] == "mount"
                        && (tokens[3] == "/system"
                        || tokens[3] == "/cache"
                        || tokens[3] == "/data")) {
                    comment_out.insert(count);
                }
            }

            ++count;
        }

        if (comment_out.empty()) {
            continue;
        }

        // Go back to beginning of file for reread
        rewind(fp.get());
        count = 0;

        std::string new_path(path);
        new_path += ".new";

        file_ptr fp_new(std::fopen(new_path.c_str(), "w"), std::fclose);
        if (!fp_new) {
            LOGE("Failed to open %s for writing: %s",
                 new_path.c_str(), strerror(errno));
            continue;
        }

        // Actually comment out the lines
        while ((read = getline(&line, &len, fp.get())) >= 0) {
            if (comment_out.find(count) != comment_out.end()) {
                fputs("#", fp_new.get());
            }
            fputs(line, fp_new.get());

            ++count;
        }

        replace_file(path.c_str(), new_path.c_str());
    }

    return true;
}

static bool fix_arter97()
{
    const char *path = "/init.mount.sh";
    const char *path_new = "/init.mount.sh.new";
    const char *fstab = "/fstab.samsungexynos7420";

    struct stat sb;
    if (stat(path, &sb) < 0 || stat(fstab, &sb) < 0) {
        return true;
    }

    file_ptr fp(std::fopen(path, "r"), std::fclose);
    if (!fp) {
        LOGE("Failed to open %s for reading: %s", path, strerror(errno));
        return false;
    }

    file_ptr fp_new(std::fopen(path_new, "w"), std::fclose);
    if (!fp_new) {
        LOGE("Failed to open %s for writing: %s", path_new, strerror(errno));
        return false;
    }

    // Strip everything from /init.mount.sh except for the shebang line
    {
        char *line = nullptr;
        size_t len = 0;
        ssize_t read = 0;

        auto free_line = util::finally([&]{
            free(line);
        });

        while ((read = getline(&line, &len, fp.get())) >= 0) {
            fputs(line, fp_new.get());
            break;
        }
    }

    fp.reset();
    fp_new.reset();

    replace_file(path, path_new);

    return true;
}

#define KLOG_CLOSE         0
#define KLOG_OPEN          1
#define KLOG_READ          2
#define KLOG_READ_ALL      3
#define KLOG_READ_CLEAR    4
#define KLOG_CLEAR         5
#define KLOG_CONSOLE_OFF   6
#define KLOG_CONSOLE_ON    7
#define KLOG_CONSOLE_LEVEL 8
#define KLOG_SIZE_UNREAD   9
#define KLOG_SIZE_BUFFER   10

static bool dump_kernel_log(const char *file)
{
    int len = klogctl(KLOG_SIZE_BUFFER, nullptr, 0);
    if (len < 0) {
        LOGE("Failed to get kernel log buffer size: %s", strerror(errno));
        return false;
    }

    char *buf = (char *) malloc(len);
    if (!buf) {
        LOGE("Failed to allocate %d bytes: %s", len, strerror(errno));
        return false;
    }

    auto free_buf = util::finally([&]{
        free(buf);
    });

    len = klogctl(KLOG_READ_ALL, buf, len);
    if (len < 0) {
        LOGE("Failed to read kernel log buffer: %s", strerror(errno));
        return false;
    }

    file_ptr fp(fopen(file, "wb"), fclose);
    if (!fp) {
        LOGE("%s: Failed to open for writing: %s", file, strerror(errno));
        return false;
    }

    if (len > 0) {
        if (fwrite(buf, len, 1, fp.get()) != 1) {
            LOGE("%s: Failed to write data: %s", file, strerror(errno));
            return false;
        }
        if (buf[len - 1] != '\n') {
            if (fputc('\n', fp.get()) == EOF) {
                LOGE("%s: Failed to write data: %s", file, strerror(errno));
                return false;
            }
        }
    }

    return true;
}

static bool emergency_reboot()
{
    static const char *log_file = "/data/media/0/MultiBoot/kernel.log";

    LOGW("--- EMERGENCY REBOOT FROM MBTOOL ---");

    // Some devices don't have /proc/last_kmsg, so we'll attempt to save the
    // kernel log to /data/media/0/MultiBoot/kernel.log
    if (!util::is_mounted("/data")) {
        LOGW("/data is not mounted. Attempting to mount /data");

        struct stat sb;

        // Try mounting /data in case we couldn't get through the fstab mounting
        // steps. (This is an ugly brute force method...)
        for (const char **ptr = data_block_devs; *ptr; ++ptr) {
            const char *block_dev = *ptr;

            if (stat(block_dev, &sb) < 0) {
                continue;
            }

            if (mount(block_dev, "/data", "ext4", 0, "") == 0
                    || mount(block_dev, "/data", "f2fs", 0, "") == 0) {
                LOGW("Mounted %s at /data", block_dev);
                break;
            }
        }
    }

    LOGW("Dumping kernel log to %s", log_file);

    // Remove old log
    remove(log_file);

    // Write new log
    util::mkdir_parent(log_file, 0775);
    dump_kernel_log(log_file);

    // Set file attributes on log file
    util::chown(log_file, "media_rw", "media_rw", 0);
    chmod(log_file, 0775);

    std::string context;
    if (util::selinux_lget_context("/data/media/0", &context)) {
        util::selinux_lset_context(log_file, context);
    }

    sync();
    umount("/data");

    // Does not return if successful
    reboot_directly("recovery");

    return false;
}

int init_main(int argc, char *argv[])
{
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            init_usage(true);
            return EXIT_SUCCESS;
        }
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
        emergency_reboot();
        return EXIT_FAILURE;
    }

    mkdir("/system", 0755);
    mkdir("/cache", 0770);
    mkdir("/data", 0771);
    util::chown("/cache", "system", "cache", 0);
    util::chown("/data", "system", "system", 0);

    fix_arter97();

    // Mount fstab and write new redacted version
    if (!mount_fstab(fstab, true)) {
        LOGE("Failed to mount fstab");
        emergency_reboot();
        return EXIT_FAILURE;
    }

    LOGE("Successfully mounted fstab");

    fix_file_contexts();
    add_mbtool_services();
    strip_manual_mounts();

    struct stat sb;
    if (stat("/sepolicy", &sb) == 0) {
        if (!patch_sepolicy("/sepolicy", "/sepolicy")) {
            LOGW("Failed to patch /sepolicy");
            emergency_reboot();
            return EXIT_FAILURE;
        }
    }

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
    emergency_reboot();
    return EXIT_FAILURE;
}

}
