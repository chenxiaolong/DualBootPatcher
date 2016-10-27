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

#include <algorithm>
#include <memory>
#include <unordered_set>

#include <cerrno>
#include <cstdlib>
#include <cstring>

#include <dirent.h>
#include <fcntl.h>
#include <getopt.h>
#include <signal.h>
#include <sys/klog.h>
#include <sys/mount.h>
#include <sys/wait.h>
#include <unistd.h>

#include "minizip/ioandroid.h"
#include "minizip/ioapi_buf.h"
#include "minizip/unzip.h"

#include "mbcommon/version.h"
#include "mbdevice/json.h"
#include "mbdevice/validate.h"
#include "mblog/kmsg_logger.h"
#include "mblog/logging.h"
#include "mbutil/autoclose/dir.h"
#include "mbutil/autoclose/file.h"
#include "mbutil/chown.h"
#include "mbutil/cmdline.h"
#include "mbutil/command.h"
#include "mbutil/delete.h"
#include "mbutil/directory.h"
#include "mbutil/file.h"
#include "mbutil/finally.h"
#include "mbutil/fstab.h"
#include "mbutil/mount.h"
#include "mbutil/path.h"
#include "mbutil/properties.h"
#include "mbutil/selinux.h"
#include "mbutil/string.h"
#include "mbutil/vibrate.h"

#include "external/property_service.h"

#include "initwrapper/devices.h"
#include "initwrapper/util.h"
#include "daemon.h"
#include "decrypt.h"
#include "file_contexts.h"
#include "mount_fstab.h"
#include "multiboot.h"
#include "reboot.h"
#include "romconfig.h"
#include "sepolpatch.h"
#include "signature.h"

#define RUN_ADB_BEFORE_EXEC_OR_REBOOT 0

#if RUN_ADB_BEFORE_EXEC_OR_REBOOT
#include "miniadbd.h"
#include "miniadbd/adb_log.h"
#endif

namespace mb
{

static pid_t daemon_pid = -1;

static void init_usage(FILE *stream)
{
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

static int wait_for_pid(const char *name, pid_t pid)
{
    int status;

    do {
        if (waitpid(pid, &status, 0) < 0) {
            LOGE("%s (pid: %d): Failed to waitpid(): %s",
                 name, pid, strerror(errno));
            return -1;
        }
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));

    if (WIFEXITED(status)) {
        LOGV("%s (pid: %d): Exited with status: %d",
             name, pid, WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        LOGV("%s (pid: %d): Killed by signal: %d",
             name, pid, WTERMSIG(status));
    }

    return status;
}

static bool start_daemon()
{
    if (daemon_pid >= 0) {
        return true;
    }

    LOGV("Starting daemon...");

    daemon_pid = fork();
    if (daemon_pid == 0) {
        execl("/proc/self/exe",
              "daemon",
              "--allow-root-client",
              "--no-patch-sepolicy",
              "--sigstop-when-ready",
              "--log-to-kmsg",
              "--no-unshare",
              nullptr);
        LOGE("Failed to exec daemon: %s", strerror(errno));
        _exit(127);
    } else if (daemon_pid > 0) {
        LOGV("Started daemon (pid: %d)", daemon_pid);
        LOGV("Waiting for SIGSTOP ready signal");

        // Wait for SIGSTOP to indicate that the daemon is ready
        int status;
        if (waitpid(daemon_pid, &status, WUNTRACED) < 0) {
            LOGW("Failed to waidpid for daemon's SIGSTOP: %s",
                 strerror(errno));
        } else if (!WIFSTOPPED(status)) {
            LOGW("waitpid returned non-SIGSTOP status");
        } else {
            LOGV("Daemon sent SIGSTOP; sending SIGCONT");
            kill(daemon_pid, SIGCONT);
        }

        return true;
    } else {
        LOGE("Failed to fork: %s", strerror(errno));
        return false;
    }
}

static bool stop_daemon()
{
    if (daemon_pid < 0) {
        return true;
    }

    LOGV("Stopping daemon...");

    // Clear pid when returning
    auto clear_pid = util::finally([]{
        daemon_pid = -1;
    });

    kill(daemon_pid, SIGTERM);

    return wait_for_pid("daemon", daemon_pid) != -1;
}

static util::CmdlineIterAction set_kernel_properties_cb(const char *name,
                                                        const char *value,
                                                        void *userdata)
{
    (void) userdata;

    if (util::starts_with(name, "androidboot.") && strlen(name) > 12 && value) {
        char buf[PROP_NAME_MAX];
        int n = snprintf(buf, sizeof(buf), "ro.boot.%s", name + 12);
        if (n >= 0 && n < (int) sizeof(buf)) {
            property_set(buf, value);
        }
    }

    return util::CmdlineIterAction::Continue;
}

static bool set_kernel_properties()
{
    util::kernel_cmdline_iter(&set_kernel_properties_cb, nullptr);

    struct {
        const char *src_prop;
        const char *dst_prop;
        const char *default_value;
    } prop_map[] = {
        { "ro.boot.serialno",   "ro.serialno",   ""        },
        { "ro.boot.mode",       "ro.bootmode",   "unknown" },
        { "ro.boot.baseband",   "ro.baseband",   "unknown" },
        { "ro.boot.bootloader", "ro.bootloader", "unknown" },
        { "ro.boot.hardware",   "ro.hardware",   "unknown" },
        { "ro.boot.revision",   "ro.revision",   "0"       },
        { nullptr,              nullptr,         nullptr   },
    };

    for (auto it = prop_map; it ->src_prop; ++it) {
        char value[PROP_VALUE_MAX];
        int ret = ::property_get(it->src_prop, value);
        property_set(it->dst_prop, (ret > 0) ? value : it->default_value);
    }

    return true;
}

static bool properties_setup()
{
    char tmp[32];
    int fd, sz;

    if (!property_init()) {
        LOGW("Failed to initialize properties area");
    }

    // Set ro.boot.* properties from the kernel command line
    if (!set_kernel_properties()) {
        LOGW("Failed to set kernel cmdline properties");
    }

    // Load /default.prop
    property_load_boot_defaults();

    // Start properties service (to allow other processes to set properties)
    if (!start_property_service()) {
        LOGW("Failed to start properties service");
    }

    get_property_workspace(&fd, &sz);
    sprintf(tmp, "%d,%d", fd, sz);

    // Clear FD_CLOEXEC since we want child processes to be able to set
    // properties
    fcntl(fd, F_SETFD, 0);

    setenv("ANDROID_PROPERTY_WORKSPACE", tmp, 1);

    return true;
}

static bool properties_cleanup()
{
    if (!stop_property_service()) {
        LOGW("Failed to stop properties service");
    }

    if (!property_cleanup()) {
        LOGW("Failed to clean up properties area");
    }

    return true;
}

static std::string get_rom_id()
{
    std::string rom_id;

    if (!util::file_first_line("/romid", &rom_id)) {
        return std::string();
    }

    return rom_id;
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

    if (chown(replace, sb.st_uid, sb.st_gid) < 0) {
        LOGE("Failed to chown %s: %s", replace, strerror(errno));
        return false;
    }

    if (chmod(replace, sb.st_mode & 0777) < 0) {
        LOGE("Failed to chmod %s: %s", replace, strerror(errno));
        return false;
    }

    return true;
}

static bool convert_binary_file_contexts()
{
    auto ret = patch_file_contexts(FILE_CONTEXTS_BIN, FILE_CONTEXTS_BIN ".new");
    if (ret == FileContextsResult::ERRNO && errno == ENOENT) {
        LOGV("%s doesn't exist; Skipping file conversion", FILE_CONTEXTS_BIN);
        return true;
    } else if (ret != FileContextsResult::OK) {
        LOGE("%s: Binary file_contexts conversion failed", FILE_CONTEXTS_BIN);
        return false;
    }

    return replace_file(FILE_CONTEXTS_BIN, FILE_CONTEXTS_BIN ".new");

    return true;
}

static bool fix_file_contexts()
{
    char path[256];
    char path_new[256];

    if (access(FILE_CONTEXTS_BIN, R_OK) == 0) {
        strlcpy(path, FILE_CONTEXTS_BIN, sizeof(path));
        strlcpy(path_new, FILE_CONTEXTS_BIN, sizeof(path_new));
    } else {
        strlcpy(path, FILE_CONTEXTS, sizeof(path));
        strlcpy(path_new, FILE_CONTEXTS, sizeof(path_new));
    }
    strlcat(path_new, ".new", sizeof(path_new));

    autoclose::file fp_old(autoclose::fopen(path, "rb"));
    if (!fp_old) {
        if (errno == ENOENT) {
            return true;
        } else {
            LOGE("%s: Failed to open for reading: %s",
                 path, strerror(errno));
            return false;
        }
    }

    autoclose::file fp_new(autoclose::fopen(path_new, "wb"));
    if (!fp_new) {
        LOGE("%s: Failed to open for writing: %s",
             path_new, strerror(errno));
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
            fputc('#', fp_new.get());
        }

        if (fwrite(line, 1, read, fp_new.get()) != (std::size_t) read) {
            LOGE("%s: Failed to write file: %s",
                 path_new, strerror(errno));
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

    return replace_file(path, path_new);
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

static bool add_mbtool_services(bool enable_appsync)
{
    autoclose::file fp_old(autoclose::fopen("/init.rc", "rb"));
    if (!fp_old) {
        if (errno == ENOENT) {
            return true;
        } else {
            LOGE("Failed to open /init.rc: %s", strerror(errno));
            return false;
        }
    }

    autoclose::file fp_new(autoclose::fopen("/init.rc.new", "wb"));
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

        if (enable_appsync) {
            if (util::starts_with(line, "service")) {
                inside_service = strstr(line, "installd") != nullptr;
            } else if (inside_service && is_completely_whitespace(line)) {
                inside_service = false;
            }

            if (inside_service && strstr(line, "disabled")) {
                has_disabled_installd = true;
            }
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
        if (enable_appsync
                && !has_disabled_installd
                && util::starts_with(line, "service")
                && strstr(line, "installd")) {
            fputs("    disabled\n", fp_new.get());
        }
    }

    if (!replace_file("/init.rc", "/init.rc.new")) {
        return false;
    }

    // Create /init.multiboot.rc
    autoclose::file fp_multiboot(autoclose::fopen("/init.multiboot.rc", "wb"));
    if (!fp_multiboot) {
        LOGE("Failed to open /init.multiboot.rc for writing: %s",
             strerror(errno));
        return false;
    }

    static const char *daemon_service =
            "service mbtooldaemon /mbtool daemon\n"
            "    class main\n"
            "    user root\n"
            "    oneshot\n"
            "    seclabel u:r:init:s0\n"
            "\n";
    static const char *appsync_service =
            "service appsync /mbtool appsync\n"
            "    class main\n"
            "    socket installd stream 600 system system\n"
            "    seclabel u:r:init:s0\n"
            "\n";

    fputs(daemon_service, fp_multiboot.get());
    if (enable_appsync) {
        fputs(appsync_service, fp_multiboot.get());
    }

    fchmod(fileno(fp_multiboot.get()), 0750);

    return true;
}

static bool strip_manual_mounts()
{
    autoclose::dir dir(autoclose::opendir("/"));
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

        autoclose::file fp(autoclose::fopen(path.c_str(), "r"));
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

        autoclose::file fp_new(autoclose::fopen(new_path.c_str(), "w"));
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

static std::string encode_list(const char * const *list)
{
    if (!list) {
        return std::string();
    }

    // We processing char-by-char, so avoid unnecessary reallocations
    std::size_t size = 0;
    std::size_t list_size = 0;
    for (auto iter = list; *iter; ++iter) {
        std::size_t item_length = strlen(*iter);
        size += item_length;
        size += std::count(*iter, *iter + item_length, ',');
        ++list_size;
    }
    if (size == 0) {
        return std::string();
    }
    size += list_size - 1;

    std::string result;
    result.reserve(size);

    bool first = true;
    for (auto iter = list; *iter; ++iter) {
        if (!first) {
            result += ',';
        } else {
            first = false;
        }
        for (auto c = *iter; *c; ++c) {
            if (*c == ',' || *c == '\\') {
                result += '\\';
            }
            result += *c;
        }
    }

    return result;
}

static bool add_props_to_default_prop(struct Device *device)
{
    autoclose::file fp(autoclose::fopen(DEFAULT_PROP_PATH, "r+b"));
    if (!fp) {
        if (errno == ENOENT) {
            return true;
        } else {
            LOGE("%s: Failed to open file: %s",
                 DEFAULT_PROP_PATH, strerror(errno));
            return false;
        }
    }

    // Add newline if the last character isn't already one
    if (std::fseek(fp.get(), -1, SEEK_END) == 0) {
        char lastchar;
        if (std::fread(&lastchar, 1, 1, fp.get()) == 1 && lastchar != '\n') {
            fputs("\n", fp.get());
        }
    } else if (std::fseek(fp.get(), 0, SEEK_END) < 0) {
        LOGE("%s: Failed to seek to end of file: %s",
             DEFAULT_PROP_PATH, strerror(errno));
        return false;
    }

    // Write version property
    fprintf(fp.get(), PROP_MULTIBOOT_VERSION "=%s\n", version());
    // Write ROM ID property
    fprintf(fp.get(), PROP_MULTIBOOT_ROM_ID "=%s\n", get_rom_id().c_str());

    // Block device paths (deprecated)
    fprintf(fp.get(), "ro.patcher.blockdevs.base=%s\n",
            encode_list(mb_device_block_dev_base_dirs(device)).c_str());
    fprintf(fp.get(), "ro.patcher.blockdevs.system=%s\n",
            encode_list(mb_device_system_block_devs(device)).c_str());
    fprintf(fp.get(), "ro.patcher.blockdevs.cache=%s\n",
            encode_list(mb_device_cache_block_devs(device)).c_str());
    fprintf(fp.get(), "ro.patcher.blockdevs.data=%s\n",
            encode_list(mb_device_data_block_devs(device)).c_str());
    fprintf(fp.get(), "ro.patcher.blockdevs.boot=%s\n",
            encode_list(mb_device_boot_block_devs(device)).c_str());
    fprintf(fp.get(), "ro.patcher.blockdevs.recovery=%s\n",
            encode_list(mb_device_recovery_block_devs(device)).c_str());
    fprintf(fp.get(), "ro.patcher.blockdevs.extra=%s\n",
            encode_list(mb_device_extra_block_devs(device)).c_str());

    if (mb_device_crypto_supported(device)) {
        fprintf(fp.get(), "ro.patcher.cryptfs_header_path=%s\n",
                mb_device_crypto_header_path(device));
    }

    return true;
}

static bool symlink_base_dir(Device *device)
{
    struct stat sb;
    if (stat(UNIVERSAL_BY_NAME_DIR, &sb) == 0) {
        return true;
    }

    std::vector<std::string> base_dirs;
    auto devs = mb_device_block_dev_base_dirs(device);
    if (devs) {
        for (auto it = devs; *it; ++it) {
            if (util::path_compare(*it, UNIVERSAL_BY_NAME_DIR) != 0
                    && stat(*it, &sb) == 0 && S_ISDIR(sb.st_mode)) {
                if (symlink(*it, UNIVERSAL_BY_NAME_DIR) < 0) {
                    LOGW("Failed to symlink %s to %s",
                         *it, UNIVERSAL_BY_NAME_DIR);
                } else {
                    LOGE("Symlinked %s to %s",
                         *it, UNIVERSAL_BY_NAME_DIR);
                    return true;
                }
            }
        }
    }

    return false;
}

static bool fstab_replace_forceencrypt(const char *path)
{
    std::string new_path(path);
    new_path += ".new";

    std::vector<util::fstab_rec> recs = util::read_fstab(path);
    if (recs.empty()) {
        LOGE("%s: Failed to read fstab file", path);
        return false;
    }

    autoclose::file fp(autoclose::fopen(new_path.c_str(), "w"));
    if (!fp) {
        LOGE("%s: Failed to open for writing: %s",
             new_path.c_str(), strerror(errno));
        return false;
    }

    for (auto &rec : recs) {
        if (rec.vold_args.find("forceencrypt") != std::string::npos) {
            util::replace_all(&rec.vold_args, "forceencrypt", "encryptable");

            fprintf(fp.get(), "%s %s %s %s %s\n", rec.blk_device.c_str(),
                    rec.mount_point.c_str(), rec.fs_type.c_str(),
                    rec.mount_args.c_str(), rec.vold_args.c_str());
        } else {
            fprintf(fp.get(), "%s\n", rec.orig_line.c_str());
        }
    }

    fp.reset();

    return replace_file(path, new_path.c_str());
}

static bool fstab_replace_block_dev(const char *path, const char *mount_point,
                                    const char *new_block_dev)
{
    std::string new_path(path);
    new_path += ".new";

    std::vector<util::fstab_rec> recs = util::read_fstab(path);
    if (recs.empty()) {
        LOGE("%s: Failed to read fstab file", path);
        return false;
    }

    autoclose::file fp(autoclose::fopen(new_path.c_str(), "w"));
    if (!fp) {
        LOGE("%s: Failed to open for writing: %s",
             new_path.c_str(), strerror(errno));
        return false;
    }

    for (auto const &rec : recs) {
        if (mount_point == rec.mount_point) {
            fprintf(fp.get(), "%s %s %s %s %s\n", new_block_dev,
                    rec.mount_point.c_str(), rec.fs_type.c_str(),
                    rec.mount_args.c_str(), rec.vold_args.c_str());
        } else {
            fprintf(fp.get(), "%s\n", rec.orig_line.c_str());
        }
    }

    fp.reset();

    return replace_file(path, new_path.c_str());
}

static std::string find_fstab()
{
    struct stat sb;

    // Try using androidboot.hardware as the fstab suffix since most devices
    // follow this scheme.
    std::string fstab("/fstab.");
    char hardware[PROP_VALUE_MAX];
    util::property_get("ro.hardware", hardware, "");
    fstab += hardware;

    if (*hardware && stat(fstab.c_str(), &sb) == 0) {
        return fstab;
    }

    // Otherwise, try to find it in the /init*.rc files
    autoclose::dir dir(autoclose::opendir("/"));
    if (!dir) {
        return std::string();
    }

    std::vector<std::string> fallback;

    struct dirent *ent;
    while ((ent = readdir(dir.get()))) {
        // Look for *.rc files
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0
                || !util::starts_with(ent->d_name, "init")
                || !util::ends_with(ent->d_name, ".rc")) {
            continue;
        }

        std::string path("/");
        path += ent->d_name;

        autoclose::file fp(autoclose::fopen(path.c_str(), "r"));
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

            if (strstr(ptr, "goldfish") || strstr(ptr, "fota")) {
                LOGV("Skipping fstab file: %s", ptr);
                continue;
            }

            fstab = ptr;

            // Replace ${ro.hardware}
            if (fstab.find("${ro.hardware}") != std::string::npos) {
                std::string hardware;
                util::kernel_cmdline_get_option("androidboot.hardware", &hardware);
                util::replace_all(&fstab, "${ro.hardware}", hardware);
            }

            LOGD("Found fstab during search: %s", fstab.c_str());

            // Check if fstab exists
            struct stat sb;
            if (stat(fstab.c_str(), &sb) < 0) {
                LOGE("Failed to stat fstab %s: %s",
                     fstab.c_str(), strerror(errno));
                continue;
            }

            // If the fstab file is for charger mode, add to fallback
            if (fstab.find("charger") != std::string::npos) {
                LOGE("Adding charger fstab to fallback fstabs: %s",
                     fstab.c_str());
                fallback.push_back(fstab);
                continue;
            }

            return fstab;
        }
    }

    if (!fallback.empty()) {
        return fallback[0];
    }
    return std::string();
}

static unsigned long get_api_version(void)
{
    std::string api_str;
    util::file_get_property("/system/build.prop",
                            "ro.build.version.sdk",
                            &api_str, "");

    char *temp;
    unsigned long api = strtoul(api_str.c_str(), &temp, 0);
    if (*temp == '\0') {
        return api;
    } else {
        return 0;
    }
}

static bool create_layout_version()
{
    // Prevent installd from dying because it can't unmount /data/media for
    // multi-user migration. Since <= 4.2 devices aren't supported anyway,
    // we'll bypass this.
    autoclose::file fp(autoclose::fopen("/data/.layout_version", "wbe"));
    if (fp) {
        const char *layout_version;
        if (get_api_version() >= 21) {
            layout_version = "3";
        } else {
            layout_version = "2";
        }

        fwrite(layout_version, 1, strlen(layout_version), fp.get());
        fp.reset();
    } else {
        LOGE("%s: Failed to open for writing: %s",
             "/data/.layout_version", strerror(errno));
        return false;
    }

    if (!util::selinux_set_context(
            "/data/.layout_version", "u:object_r:install_data_file:s0")) {
        LOGE("%s: Failed to set SELinux context: %s",
             "/data/.layout_version", strerror(errno));
        return false;
    }

    return true;
}

bool mount_userdata(const char *block_dev)
{
    std::string rom_id = get_rom_id();
    std::shared_ptr<Rom> rom = Roms::create_rom(rom_id);
    if (!rom) {
        return false;
    }

    std::string fstab(find_fstab());

    if (fstab.empty()) {
        LOGE("Failed to find fstab");
        return false;
    }

    // Rewrite fstab with the devmapper device for /data
    fstab_replace_block_dev(fstab.c_str(), "/data", block_dev);

    // Try mounting data again
    int flags = MOUNT_FLAG_REWRITE_FSTAB | MOUNT_FLAG_MOUNT_DATA;
    // nullptr for the device because we do not need the generic fstab entries
    if (!mount_fstab(fstab.c_str(), rom, nullptr, flags)) {
        LOGE("Failed to mount data. Decryption probably failed");
        return false;
    }

    return true;
}

static bool extract_zip(const char *source, const char *target)
{
    unzFile uf;
    zlib_filefunc64_def zFunc;
    ourbuffer_t iobuf;

    memset(&zFunc, 0, sizeof(zFunc));
    memset(&iobuf, 0, sizeof(iobuf));

    fill_android_filefunc64(&iobuf.filefunc64);
    fill_buffer_filefunc64(&zFunc, &iobuf);

    uf = unzOpen2_64(source, &zFunc);
    if (!uf) {
        LOGE("%s: Failed to open zip", source);
        return false;
    }

    auto close_zip = util::finally([&]{
        unzClose(uf);
    });

    if (unzLocateFile(uf, "exec", nullptr) != UNZ_OK) {
        LOGE("%s: Failed to find 'exec' in zip", source);
        return false;
    }

    if (unzOpenCurrentFile(uf) != UNZ_OK) {
        LOGE("%s: Failed to open file in zip", source);
        return false;
    }

    auto close_inner_file = util::finally([&]{
        unzCloseCurrentFile(uf);
    });

    std::string target_file(target);
    target_file += "/exec";

    util::mkdir_recursive(target, 0755);

    FILE *fp = fopen(target_file.c_str(), "wb");
    if (!fp) {
        LOGE("%s: Failed to open for writing: %s",
             target_file.c_str(), strerror(errno));
        return false;
    }

    char buf[8192];
    int bytes_read;

    while ((bytes_read = unzReadCurrentFile(uf, buf, sizeof(buf))) > 0) {
        size_t bytes_written = fwrite(buf, 1, bytes_read, fp);
        if (bytes_written == 0) {
            bool ret = !ferror(fp);
            fclose(fp);
            return ret;
        }
    }
    if (bytes_read != 0) {
        LOGE("%s: Failed before reaching inner file's EOF", source);
        fclose(fp);
        return false;
    }

    if (fclose(fp) < 0) {
        LOGE("%s: Error when closing file: %s",
             target_file.c_str(), strerror(errno));
        return false;
    }

    return true;
}

static bool launch_boot_menu(bool has_encryption)
{
    struct stat sb;
    bool skip = false;

    // Boot UI must always run if the device is encrypted
    if (!has_encryption && stat(BOOT_UI_SKIP_PATH, &sb) == 0) {
        std::string skip_rom;
        util::file_first_line(BOOT_UI_SKIP_PATH, &skip_rom);

        std::string rom_id = get_rom_id();

        if (skip_rom == rom_id) {
            LOGV("Performing one-time skipping of Boot UI");
            skip = true;
        } else {
            LOGW("Skip file is not for: %s", rom_id.c_str());
            LOGW("Not skipping boot UI");
        }
    }

    if (remove(BOOT_UI_SKIP_PATH) < 0 && errno != ENOENT) {
        LOGW("%s: Failed to remove file: %s",
             BOOT_UI_SKIP_PATH, strerror(errno));
        LOGW("Boot UI won't run again!");
    }

    if (skip) {
        return true;
    }

    if (stat(BOOT_UI_ZIP_PATH, &sb) < 0) {
        LOGV("Boot UI is missing. Skipping...");
        return true;
    }

    // Verify boot UI signature
    SigVerifyResult result;
    result = verify_signature(BOOT_UI_ZIP_PATH, BOOT_UI_ZIP_PATH ".sig");
    if (result != SigVerifyResult::VALID) {
        LOGE("%s: Invalid signature", BOOT_UI_ZIP_PATH);
        return false;
    }

    if (!extract_zip(BOOT_UI_ZIP_PATH, BOOT_UI_PATH)) {
        LOGE("%s: Failed to extract zip", BOOT_UI_ZIP_PATH);
        return false;
    }

    auto clean_up = util::finally([]{
        if (!util::delete_recursive(BOOT_UI_PATH)) {
            LOGW("%s: Failed to recursively delete: %s",
                 BOOT_UI_PATH, strerror(errno));
        }
    });

    if (chmod(BOOT_UI_EXEC_PATH, 0500) < 0) {
        LOGE("%s: Failed to chmod: %s", BOOT_UI_EXEC_PATH, strerror(errno));
        return false;
    }

    start_daemon();

    const char *argv[] = { BOOT_UI_EXEC_PATH, BOOT_UI_ZIP_PATH, nullptr };
    int ret = util::run_command(argv[0], argv, nullptr, nullptr, nullptr,
                                nullptr);
    if (ret < 0) {
        LOGE("%s: Failed to execute: %s", BOOT_UI_EXEC_PATH, strerror(errno));
    } else if (WIFEXITED(ret)) {
        LOGV("Boot UI exited with status: %d", WEXITSTATUS(ret));
    } else if (WIFSIGNALED(ret)) {
        LOGE("Boot UI killed by signal: %d", WTERMSIG(ret));
    }

    // NOTE: Always continue regardless of how the boot UI exits. If the current
    // ROM should be booted, the UI simply exits. If the UI crashes or doesn't
    // work for whatever reason, the current ROM should still be booted. If the
    // user switches to another ROM, then the boot UI will instruct the daemon
    // to switch ROMs and reboot (ie. the UI will not exit).

    stop_daemon();

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

    autoclose::file fp(autoclose::fopen(file, "wb"));
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

#if RUN_ADB_BEFORE_EXEC_OR_REBOOT
static void run_adb()
{
    // Mount /system if we can so we can use adb shell
    if (!util::is_mounted("/system") && util::is_mounted("/raw/system")) {
        mount("/raw/system", "/system", "", MS_BIND, "");
    }

    pid_t pid = fork();
    if (pid == 0) {
        // Don't spam the kernel log
        adb_log_mask = ADB_SERV;

        char *adb_argv[] = { const_cast<char *>("miniadbd"), nullptr };
        _exit(miniadbd_main(1, adb_argv));
    } else if (pid >= 0) {
        LOGV("miniadbd is running as pid %d; kill it to continue", pid);
        wait_for_pid("miniadbd", pid);
    } else {
        LOGW("Failed to fork to run miniadbd: %s", strerror(errno));
    }
}
#endif

static bool emergency_reboot()
{
#if RUN_ADB_BEFORE_EXEC_OR_REBOOT
    run_adb();
#endif

    util::vibrate(100, 150);
    util::vibrate(100, 150);
    util::vibrate(100, 150);
    util::vibrate(100, 150);
    util::vibrate(100, 150);

    LOGW("--- EMERGENCY REBOOT FROM MBTOOL ---");

    // Some devices don't have /proc/last_kmsg, so we'll attempt to save the
    // kernel log to /data/media/0/MultiBoot/kernel.log
    if (!util::is_mounted("/data")) {
        LOGW("/data is not mounted. Attempting to mount /data");

        struct stat sb;

        // Try mounting /data in case we couldn't get through the fstab mounting
        // steps. (This is an ugly brute force method...)
        std::vector<std::string> data_block_devs;

        std::vector<unsigned char> contents;
        util::file_read_all(DEVICE_JSON_PATH, &contents);
        contents.push_back('\0');

        MbDeviceJsonError error;
        Device *device = mb_device_new_from_json(
                (char *) contents.data(), &error);
        if (device) {
            auto devs = mb_device_data_block_devs(device);
            if (devs) {
                for (auto it = devs; *it; ++it) {
                    data_block_devs.push_back(*it);
                }
            }
            mb_device_free(device);
        }

        // Some catch-all paths to increase our chances of getting a usable log
        data_block_devs.push_back(UNIVERSAL_BY_NAME_DIR "/data");
        data_block_devs.push_back(UNIVERSAL_BY_NAME_DIR "/DATA");
        data_block_devs.push_back(UNIVERSAL_BY_NAME_DIR "/userdata");
        data_block_devs.push_back(UNIVERSAL_BY_NAME_DIR "/USERDATA");
        data_block_devs.push_back(UNIVERSAL_BY_NAME_DIR "/UDA");

        for (const std::string &block_dev : data_block_devs) {
            if (stat(block_dev.c_str(), &sb) < 0) {
                continue;
            }

            if (mount(block_dev.c_str(), "/data", "ext4", 0, "") == 0
                    || mount(block_dev.c_str(), "/data", "f2fs", 0, "") == 0) {
                LOGW("Mounted %s at /data", block_dev.c_str());
                break;
            }
        }
    }

    LOGW("Dumping kernel log to %s", MULTIBOOT_LOG_KERNEL);

    // Remove old log
    remove(MULTIBOOT_LOG_KERNEL);

    // Write new log
    util::mkdir_parent(MULTIBOOT_LOG_KERNEL, 0775);
    dump_kernel_log(MULTIBOOT_LOG_KERNEL);

    // Set file attributes on log file
    fix_multiboot_permissions();

    sync();
    umount("/data");

    // Does not return if successful
    reboot_directly("recovery");

    return false;
}

int init_main(int argc, char *argv[])
{
    // Some devices actually receive arguments, so ignore them during boot
    if (getppid() != 0) {
        static struct option long_options[] = {
            {"help", no_argument, 0, 'h'},
            {0, 0, 0, 0}
        };

        int opt;
        int long_index = 0;

        while ((opt = getopt_long(argc, argv, "h", long_options, &long_index)) != -1) {
            switch (opt) {
            case 'h':
                init_usage(stdout);
                return EXIT_SUCCESS;

            default:
                init_usage(stderr);
                return EXIT_FAILURE;
            }
        }

        // There should be no other arguments
        if (argc - optind != 0) {
            init_usage(stderr);
            return EXIT_FAILURE;
        }
    }

    // Mount base directories
    mkdir("/dev", 0755);
    mkdir("/proc", 0755);
    mkdir("/sys", 0755);

    mount("tmpfs", "/dev", "tmpfs", MS_NOSUID, "mode=0755");
    mkdir("/dev/pts", 0755);
    mkdir("/dev/socket", 0755);
    mount("devpts", "/dev/pts", "devpts", 0, nullptr);
    mount("proc", "/proc", "proc", 0, nullptr);
    mount("sysfs", "/sys", "sysfs", 0, nullptr);

    // Create mount points
    mkdir("/system", 0755);
    mkdir("/cache", 0770);
    mkdir("/data", 0771);
    util::chown("/cache", "system", "cache", 0);
    util::chown("/data", "system", "system", 0);

    // Redirect std{in,out,err} to /dev/null
    open_devnull_stdio();

    // Log to kmsg
    log::log_set_logger(std::make_shared<log::KmsgLogger>(true));
    if (klogctl(KLOG_CONSOLE_LEVEL, nullptr, 7) < 0) {
        LOGE("Failed to set loglevel: %s", strerror(errno));
    }

    LOGV("Booting up with version %s (%s)",
         version(), git_version());

    std::vector<unsigned char> contents;
    util::file_read_all(DEVICE_JSON_PATH, &contents);
    contents.push_back('\0');

    MbDeviceJsonError error;
    Device *device = mb_device_new_from_json((char *) contents.data(), &error);
    if (!device) {
        LOGE("%s: Failed to load device definition", DEVICE_JSON_PATH);
        emergency_reboot();
        return EXIT_FAILURE;
    } else if (mb_device_validate(device) != 0) {
        LOGE("%s: Device definition validation failed", DEVICE_JSON_PATH);
        emergency_reboot();
        return EXIT_FAILURE;
    }

    add_props_to_default_prop(device);

    // initialize properties
    properties_setup();

    // Start probing for devices
    device_init(false);

    // Symlink by-name directory to /dev/block/by-name (ugh... ASUS)
    symlink_base_dir(device);

    std::string fstab(find_fstab());

    LOGV("fstab file: %s", fstab.c_str());

    if (access(fstab.c_str(), R_OK) < 0) {
        LOGW("%s: Failed to access file: %s", fstab.c_str(), strerror(errno));
        LOGW("Continuing anyway...");
        fstab = "/fstab.MBTOOL_DUMMY_DO_NOT_USE";
        util::create_empty_file(fstab);
    }

    // Replace forceencrypt in fstab file since vold will need to process it if
    // /data is encrypted. mbtool's mount code doesn't care about the presence
    // of this option.
    fstab_replace_forceencrypt(fstab.c_str());

    // Get ROM ID from /romid
    std::string rom_id = get_rom_id();
    std::shared_ptr<Rom> rom = Roms::create_rom(rom_id);
    if (!rom) {
        LOGE("Unknown ROM ID: %s", rom_id.c_str());
        emergency_reboot();
        return EXIT_FAILURE;
    }

    LOGV("ROM ID is: %s", rom_id.c_str());

    // Mount system, cache, and external SD from fstab file
    int flags = MOUNT_FLAG_REWRITE_FSTAB
            | MOUNT_FLAG_MOUNT_SYSTEM
            | MOUNT_FLAG_MOUNT_CACHE
            | MOUNT_FLAG_MOUNT_EXTERNAL_SD;
    if (!mount_fstab(fstab.c_str(), rom, device, flags)) {
        LOGE("Failed to mount fstab");
        emergency_reboot();
        return EXIT_FAILURE;
    }

    LOGV("Successfully mounted fstab (excluding /data)");

    bool has_encryption = false;

    // Try mounting data
    flags = MOUNT_FLAG_REWRITE_FSTAB
            | MOUNT_FLAG_MOUNT_DATA;
    if (!mount_fstab(fstab.c_str(), rom, device, flags)) {
        LOGW("Failed to mount data, it might be encrypted");
        has_encryption = true;
    }

    property_set(PROP_CRYPTO_STATE,
                 has_encryption
                 ? CRYPTO_STATE_ENCRYPTED
                 : CRYPTO_STATE_DECRYPTED);

    if (!launch_boot_menu(has_encryption)) {
        LOGE("Failed to run boot menu");
        // Continue anyway since boot menu might not run on every device
    }

    // Check if the data partition was successfully decrypted
    if (has_encryption) {
        char value[PROP_VALUE_MAX];
        if (::property_get(PROP_CRYPTO_STATE, value) <= 0
                || strcmp(value, CRYPTO_STATE_DECRYPTED) != 0) {
            LOGE("Failed to decrypt device");
            emergency_reboot();
            return EXIT_FAILURE;
        }

        if (!util::is_mounted("/raw/data")) {
            LOGE("/raw/data did not get mounted");
            emergency_reboot();
            return EXIT_FAILURE;
        }
    }

    // Mount selinuxfs
    util::selinux_mount();

    // Mount ROM (bind mount directory or mount images, etc.)
    if (!mount_rom(rom)) {
        LOGE("Failed to mount ROM directories and images");
        emergency_reboot();
        return EXIT_FAILURE;
    }

    std::string config_path(rom->config_path());
    RomConfig config;
    if (!config.load_file(config_path)) {
        LOGW("%s: Failed to load config for ROM %s",
             config_path.c_str(), rom->id.c_str());
    }

    // Make runtime ramdisk modifications
    convert_binary_file_contexts();
    fix_file_contexts();
    add_mbtool_services(config.indiv_app_sharing);
    strip_manual_mounts();

    // Data modifications
    create_layout_version();

    // Patch SELinux policy
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

    // Kill properties service and clean up
    properties_cleanup();

    // Remove mbtool init symlink and restore original binary
    unlink("/init");
    rename("/init.orig", "/init");

#if RUN_ADB_BEFORE_EXEC_OR_REBOOT
    run_adb();
#endif

    // Unmount partitions
    util::selinux_unmount();
    umount("/dev/pts");
    umount("/dev");
    umount("/proc");
    umount("/sys");
    // Do not remove these as Android 6.0 init's stage 1 no longer creates these
    // (platform/system/core commit a1f6a4b13921f61799be14a2544bdbf95958eae7)
    //rmdir("/dev");
    //rmdir("/proc");
    //rmdir("/sys");

    // Start real init
    LOGD("Launching real init ...");
    execlp("/init", "/init", nullptr);
    LOGE("Failed to exec real init: %s", strerror(errno));
    emergency_reboot();
    return EXIT_FAILURE;
}

}
