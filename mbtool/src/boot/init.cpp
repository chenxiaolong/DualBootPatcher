/*
 * Copyright (C) 2015-2018  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "boot/init.h"

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
#include <sys/sysmacros.h>
#include <sys/wait.h>
#include <unistd.h>

#include "mz.h"
#include "mz_strm_android.h"
#include "mz_strm_buf.h"
#include "mz_zip.h"

#include "mbcommon/finally.h"
#include "mbcommon/string.h"
#include "mbcommon/version.h"
#include "mbdevice/json.h"
#include "mblog/kmsg_logger.h"
#include "mblog/logging.h"
#include "mbutil/chown.h"
#include "mbutil/cmdline.h"
#include "mbutil/command.h"
#include "mbutil/delete.h"
#include "mbutil/directory.h"
#include "mbutil/file.h"
#include "mbutil/fstab.h"
#include "mbutil/mount.h"
#include "mbutil/path.h"
#include "mbutil/properties.h"
#include "mbutil/selinux.h"
#include "mbutil/string.h"

#include "boot/daemon.h"
#include "boot/emergency.h"
#include "boot/mount_fstab.h"
#include "boot/property_service.h"
#include "boot/uevent_thread.h"
#include "util/multiboot.h"
#include "util/romconfig.h"
#include "util/sepolpatch.h"
#include "util/signature.h"

#if defined(__i386__) || defined(__arm__)
#define PCRE_PATH               "/system/lib/libpcre.so"
#elif defined(__x86_64__) || defined(__aarch64__)
#define PCRE_PATH               "/system/lib64/libpcre.so"
#else
#error Unknown PCRE path for architecture
#endif

#define LOG_TAG "mbtool/boot/init"

using namespace mb::device;

namespace mb
{

using ScopedDIR = std::unique_ptr<DIR, decltype(closedir) *>;
using ScopedFILE = std::unique_ptr<FILE, decltype(fclose) *>;

static pid_t daemon_pid = -1;

static PropertyService g_property_service;

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
    auto clear_pid = finally([]{
        daemon_pid = -1;
    });

    kill(daemon_pid, SIGTERM);

    return wait_for_pid("daemon", daemon_pid) != -1;
}

static bool set_kernel_properties()
{
    if (auto cmdline = util::kernel_cmdline()) {
        for (auto const &[k, v] : cmdline.value()) {
            LOGV("Kernel cmdline option %s=%s",
                 k.c_str(), v ? v->c_str() : "(no value)");

            if (starts_with(k, "androidboot.") && k.size() > 12 && v) {
                std::string key("ro.boot.");
                key += std::string_view(k).substr(12);
                g_property_service.set(key, *v);
            }
        }
    } else {
        LOGW("Failed get kernel cmdline: %s",
             cmdline.error().message().c_str());
    }

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
    };

    for (auto const &p : prop_map) {
        auto value = g_property_service.get(p.src_prop);
        g_property_service.set(p.dst_prop, value ? *value : p.default_value);
    }

    return true;
}

static bool properties_setup()
{
    if (!g_property_service.initialize()) {
        LOGW("Failed to initialize properties area");
    }

    // Set ro.boot.* properties from the kernel command line
    if (!set_kernel_properties()) {
        LOGW("Failed to set kernel cmdline properties");
    }

    // Load /default.prop
    g_property_service.load_boot_props();

    // Load DBP props
    g_property_service.load_properties_file(DBP_PROP_PATH, {});

    // Start properties service (to allow other processes to set properties)
    if (!g_property_service.start_thread()) {
        LOGW("Failed to start properties service");
    }

    return true;
}

static bool properties_cleanup()
{
    if (!g_property_service.stop_thread()) {
        LOGW("Failed to stop properties service");
    }

    return true;
}

static std::string get_rom_id()
{
    auto rom_id = util::file_first_line("/romid");
    if (!rom_id) {
        return {};
    }

    return std::move(rom_id.value());
}

// Operating on paths instead of fd's should be safe enough since, at this
// point, we're the only process alive on the system.
static bool replace_file(const char *replace, const char *with)
{
    struct stat sb;
    int sb_ret;

    sb_ret = stat(replace, &sb);

    if (rename(with, replace) < 0) {
        LOGE("Failed to rename %s to %s: %s", with, replace, strerror(errno));
        return false;
    }

    if (sb_ret == 0) {
        if (chown(replace, sb.st_uid, sb.st_gid) < 0) {
            LOGE("Failed to chown %s: %s", replace, strerror(errno));
            return false;
        }

        if (chmod(replace, sb.st_mode & 0777) < 0) {
            LOGE("Failed to chmod %s: %s", replace, strerror(errno));
            return false;
        }
    }

    return true;
}

static bool fix_file_contexts(const char *path)
{
    std::string new_path(path);
    new_path += ".new";

    ScopedFILE fp_old(fopen(path, "rbe"), fclose);
    if (!fp_old) {
        if (errno == ENOENT) {
            return true;
        } else {
            LOGE("%s: Failed to open for reading: %s",
                 path, strerror(errno));
            return false;
        }
    }

    ScopedFILE fp_new(fopen(new_path.c_str(), "wbe"), fclose);
    if (!fp_new) {
        LOGE("%s: Failed to open for writing: %s",
             new_path.c_str(), strerror(errno));
        return false;
    }

    char *line = nullptr;
    size_t len = 0;
    ssize_t read = 0;

    auto free_line = finally([&]{
        free(line);
    });

    while ((read = getline(&line, &len, fp_old.get())) >= 0) {
        if (starts_with(line, "/data/media(") && !strstr(line, "<<none>>")) {
            fputc('#', fp_new.get());
        }

        if (fwrite(line, 1, static_cast<size_t>(read), fp_new.get())
                != static_cast<size_t>(read)) {
            LOGE("%s: Failed to write file: %s",
                 new_path.c_str(), strerror(errno));
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

    return replace_file(path, new_path.c_str());
}

static bool fix_binary_file_contexts(const char *path)
{
    std::string new_path(path);
    new_path += ".bin";
    std::string tmp_path(path);
    tmp_path += ".tmp";

    // Check signature
    SigVerifyResult result;
    result = verify_signature("/sbin/file-contexts-tool",
                              "/sbin/file-contexts-tool.sig");
    if (result != SigVerifyResult::Valid) {
        LOGE("%s: Invalid signature", "/sbin/file-contexts-tool");
        return false;
    }

    std::vector<std::string> decompile_argv{
        "/sbin/file-contexts-tool",  "decompile", "-p", PCRE_PATH,
        path, tmp_path
    };
    std::vector<std::string> compile_argv{
        "/sbin/file-contexts-tool", "compile", "-p", PCRE_PATH,
        tmp_path, new_path
    };

    // Decompile binary file_contexts to temporary file
    int ret = util::run_command(decompile_argv[0], decompile_argv, {}, {}, {});
    if (ret < 0 || !WIFEXITED(ret) || WEXITSTATUS(ret) != 0) {
        LOGE("%s: Failed to decompile file_contexts", path);
        return false;
    }

    // Patch temporary file
    if (!fix_file_contexts(tmp_path.c_str())) {
        unlink(tmp_path.c_str());
        return false;
    }

    // Recompile binary file_contexts
    ret = util::run_command(compile_argv[0], compile_argv, {}, {}, {});
    if (ret < 0 || !WIFEXITED(ret) || WEXITSTATUS(ret) != 0) {
        LOGE("%s: Failed to compile binary file_contexts", tmp_path.c_str());
        unlink(tmp_path.c_str());
        return false;
    }

    unlink(tmp_path.c_str());

    return replace_file(path, new_path.c_str());
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
    ScopedFILE fp_old(fopen("/init.rc", "rbe"), fclose);
    if (!fp_old) {
        if (errno == ENOENT) {
            return true;
        } else {
            LOGE("Failed to open /init.rc: %s", strerror(errno));
            return false;
        }
    }

    ScopedFILE fp_new(fopen("/init.rc.new", "wbe"), fclose);
    if (!fp_new) {
        LOGE("Failed to open /init.rc.new for writing: %s",
             strerror(errno));
        return false;
    }

    char *line = nullptr;
    size_t len = 0;
    ssize_t read = 0;

    auto free_line = finally([&]{
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
            if (starts_with(line, "service")) {
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

        if (fwrite(line, 1, static_cast<size_t>(read), fp_new.get())
                != static_cast<size_t>(read)) {
            LOGE("Failed to write to /init.rc.new: %s", strerror(errno));
            return false;
        }

        // Disable installd. mbtool's appsync will spawn it on demand
        if (enable_appsync
                && !has_disabled_installd
                && starts_with(line, "service")
                && strstr(line, "installd")) {
            fputs("    disabled\n", fp_new.get());
        }
    }

    if (!replace_file("/init.rc", "/init.rc.new")) {
        return false;
    }

    // Create /init.multiboot.rc
    ScopedFILE fp_multiboot(fopen("/init.multiboot.rc", "wbe"), fclose);
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
            "    seclabel " MB_EXEC_CONTEXT "\n"
            "\n"
            "service mbtoolprops /mbtool properties set-file " DBP_PROP_PATH "\n"
            "    class core\n"
            "    user root\n"
            "    oneshot\n"
            "    seclabel " MB_EXEC_CONTEXT "\n"
            "\n";
    static const char *appsync_service =
            "service appsync /mbtool appsync\n"
            "    class main\n"
            "    socket installd stream 600 system system\n"
            "    seclabel " MB_EXEC_CONTEXT "\n"
            "\n";

    fputs(daemon_service, fp_multiboot.get());
    if (enable_appsync) {
        fputs(appsync_service, fp_multiboot.get());
    }

    fchmod(fileno(fp_multiboot.get()), 0750);

    return true;
}

static bool write_fstab_hack(const char *fstab)
{
    ScopedFILE fp_fstab(fopen(fstab, "abe"), fclose);
    if (!fp_fstab) {
        LOGE("%s: Failed to open for writing: %s",
             fstab, strerror(errno));
        return false;
    }

    fputs(R"EOF(
# The following is added to prevent vold in Android 7.0 from segfaulting due to
# dereferencing a null pointer when checking if the /data fstab entry has the
# "forcefdeorfbe" vold option. (See cryptfs_isConvertibleToFBE() in
# system/vold/cryptfs.c.)

/dev/null /data auto defaults voldmanaged=dummy:auto
)EOF", fp_fstab.get());

    return true;
}

static bool strip_manual_mounts()
{
    ScopedDIR dir(opendir("/"), closedir);
    if (!dir) {
        return true;
    }

    struct dirent *ent;
    while ((ent = readdir(dir.get()))) {
        // Look for *.rc files
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0
                || !ends_with(ent->d_name, ".rc")) {
            continue;
        }

        std::string path("/");
        path += ent->d_name;

        ScopedFILE fp(fopen(path.c_str(), "re"), fclose);
        if (!fp) {
            LOGE("Failed to open %s for reading: %s",
                 path.c_str(), strerror(errno));
            continue;
        }

        char *line = nullptr;
        size_t len = 0;
        ssize_t read = 0;

        auto free_line = finally([&]{
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

        ScopedFILE fp_new(fopen(new_path.c_str(), "we"), fclose);
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

static bool add_props_to_dbp_prop()
{
    ScopedFILE fp(fopen(DBP_PROP_PATH, "abe"), fclose);
    if (!fp) {
        LOGE("%s: Failed to open file: %s",
             DBP_PROP_PATH, strerror(errno));
        return false;
    }

    // Write version property
    fprintf(fp.get(), PROP_MULTIBOOT_VERSION "=%s\n", version());
    // Write ROM ID property
    fprintf(fp.get(), PROP_MULTIBOOT_ROM_ID "=%s\n", get_rom_id().c_str());

    return true;
}

static bool symlink_base_dir(const Device &device)
{
    struct stat sb;
    if (stat(UNIVERSAL_BY_NAME_DIR, &sb) == 0) {
        return true;
    }

    for (auto const &path : device.block_dev_base_dirs()) {
        if (util::path_compare(path, UNIVERSAL_BY_NAME_DIR) != 0
                && stat(path.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode)) {
            if (symlink(path.c_str(), UNIVERSAL_BY_NAME_DIR) < 0) {
                LOGW("Failed to symlink %s to %s",
                     path.c_str(), UNIVERSAL_BY_NAME_DIR);
            } else {
                LOGE("Symlinked %s to %s",
                     path.c_str(), UNIVERSAL_BY_NAME_DIR);
                return true;
            }
        }
    }

    return false;
}

static std::string find_fstab()
{
    struct stat sb;

    // Try using androidboot.hardware as the fstab suffix since most devices
    // follow this scheme.
    std::string fstab("/fstab.");
    std::string hardware;
    hardware = util::property_get_string("ro.hardware", "");
    fstab += hardware;

    if (!hardware.empty() && stat(fstab.c_str(), &sb) == 0) {
        return fstab;
    }

    // Otherwise, try to find it in the /init*.rc files
    ScopedDIR dir(opendir("/"), closedir);
    if (!dir) {
        return std::string();
    }

    std::vector<std::string> fallback;

    struct dirent *ent;
    while ((ent = readdir(dir.get()))) {
        // Look for *.rc files
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0
                || !starts_with(ent->d_name, "init")
                || !ends_with(ent->d_name, ".rc")) {
            continue;
        }

        std::string path("/");
        path += ent->d_name;

        ScopedFILE fp(fopen(path.c_str(), "re"), fclose);
        if (!fp) {
            continue;
        }

        char *line = nullptr;
        size_t len = 0;
        ssize_t read = 0;

        auto free_line = finally([&]{
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
                util::replace_all(fstab, "${ro.hardware}", hardware);
            }

            LOGD("Found fstab during search: %s", fstab.c_str());

            // Check if fstab exists
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

static unsigned long get_api_version()
{
    return util::property_file_get_num<unsigned long>(
            "/system/build.prop", "ro.build.version.sdk", 0);
}

static bool create_layout_version()
{
    // Prevent installd from dying because it can't unmount /data/media for
    // multi-user migration. Since <= 4.2 devices aren't supported anyway,
    // we'll bypass this.
    ScopedFILE fp(fopen("/data/.layout_version", "wbe"), fclose);
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

    if (auto ret = util::selinux_set_context("/data/.layout_version",
            "u:object_r:install_data_file:s0"); !ret) {
        LOGE("%s: Failed to set SELinux context: %s",
             "/data/.layout_version", ret.error().message().c_str());
        return false;
    }

    return true;
}

static bool disable_installd()
{
    static constexpr char installd_init[] = "/system/etc/init/installd.rc";

    if (access(installd_init, R_OK) < 0) {
        if (errno == ENOENT) {
            LOGV("%s: installd init file not found", installd_init);
            return true;
        } else {
            LOGE("%s: Failed to access file: %s",
                 installd_init, strerror(errno));
            return false;
        }
    }

    if (auto ret = util::mount(
            "/dev/null", installd_init, "", MS_BIND | MS_RDONLY, ""); !ret) {
        LOGE("%s: Failed to bind mount /dev/null: %s",
             installd_init, ret.error().message().c_str());
        return false;
    }

    return true;
}

static bool disable_spota()
{
    static const char *spota_dir = "/data/security/spota";

    auto props = util::property_file_get_all("/system/build.prop");

    if (props && strcasecmp((*props)["ro.product.brand"].c_str(), "samsung") != 0
            && strcasecmp((*props)["ro.product.manufacturer"].c_str(), "samsung") != 0) {
        // Not a Samsung device
        LOGV("Not mounting empty tmpfs over: %s", spota_dir);
        return true;
    }

    if (auto r = util::mkdir_recursive(spota_dir, 0);
            !r && r.error() != std::errc::file_exists) {
        LOGE("%s: Failed to create directory: %s", spota_dir,
             r.error().message().c_str());
        return false;
    }

    if (auto ret = util::mount(
            "tmpfs", spota_dir, "tmpfs", MS_RDONLY, "mode=0000"); !ret) {
        LOGE("%s: Failed to mount tmpfs: %s",
             spota_dir, ret.error().message().c_str());
        return false;
    }

    LOGV("%s: Mounted read-only tmpfs over spota directory", spota_dir);

    return true;
}

static bool extract_zip(const char *source, const char *target)
{
    void *stream;
    if (!mz_stream_android_create(&stream)) {
        LOGE("Failed to create base stream");
        return false;
    }

    auto destroy_stream = finally([&] {
        mz_stream_delete(&stream);
    });

    void *buf_stream;
    if (!mz_stream_buffered_create(&buf_stream)) {
        LOGE("Failed to create buffered stream");
        return false;
    }

    auto destroy_buf_stream = finally([&] {
        mz_stream_delete(&buf_stream);
    });

    if (mz_stream_set_base(buf_stream, stream) != MZ_OK) {
        LOGE("Failed to set base stream for buffered stream");
        return false;
    }

    if (mz_stream_open(buf_stream, source, MZ_OPEN_MODE_READ) != MZ_OK) {
        LOGE("%s: Failed to open stream", source);
        return false;
    }

    auto close_stream = finally([&] {
        mz_stream_close(buf_stream);
    });

    auto *handle = mz_zip_open(buf_stream, MZ_OPEN_MODE_READ);
    if (!handle) {
        LOGE("%s: Failed to open zip", source);
        return false;
    }

    auto close_zip = finally([&]{
        mz_zip_close(handle);
    });

    if (mz_zip_locate_entry(handle, "exec", nullptr) != MZ_OK) {
        LOGE("%s: Failed to find 'exec' in zip", source);
        return false;
    }

    if (mz_zip_entry_read_open(handle, 0, nullptr) != MZ_OK) {
        LOGE("%s: Failed to open file in zip", source);
        return false;
    }

    auto close_inner_file = finally([&] {
        mz_zip_entry_close(handle);
    });

    std::string target_file(target);
    target_file += "/exec";

    (void) util::mkdir_recursive(target, 0755);

    FILE *fp = fopen(target_file.c_str(), "wbe");
    if (!fp) {
        LOGE("%s: Failed to open for writing: %s",
             target_file.c_str(), strerror(errno));
        return false;
    }

    auto close_fp = finally([&] {
        fclose(fp);
    });

    char buf[UINT16_MAX];
    int bytes_read;

    while ((bytes_read = mz_zip_entry_read(handle, buf, sizeof(buf))) > 0) {
        size_t bytes_written = fwrite(buf, 1, static_cast<size_t>(bytes_read),
                                      fp);
        if (bytes_written == 0) {
            LOGE("%s: Truncated write", target);
            return false;
        }
    }
    if (bytes_read != 0) {
        LOGE("%s: Failed before reaching inner file's EOF", source);
        return false;
    }

    close_fp.dismiss();

    if (fclose(fp) < 0) {
        LOGE("%s: Error when closing file: %s",
             target_file.c_str(), strerror(errno));
        return false;
    }

    return true;
}

static bool launch_boot_menu()
{
    struct stat sb;
    bool skip = false;

    if (stat(BOOT_UI_SKIP_PATH, &sb) == 0) {
        auto skip_rom = util::file_first_line(BOOT_UI_SKIP_PATH);

        std::string rom_id = get_rom_id();

        if (skip_rom && skip_rom.value() == rom_id) {
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
    if (result != SigVerifyResult::Valid) {
        LOGE("%s: Invalid signature", BOOT_UI_ZIP_PATH);
        return false;
    }

    if (!extract_zip(BOOT_UI_ZIP_PATH, BOOT_UI_PATH)) {
        LOGE("%s: Failed to extract zip", BOOT_UI_ZIP_PATH);
        return false;
    }

    auto clean_up = finally([]{
        if (auto r = util::delete_recursive(BOOT_UI_PATH); !r) {
            LOGW("%s: Failed to recursively delete: %s",
                 BOOT_UI_PATH, r.error().message().c_str());
        }
    });

    if (chmod(BOOT_UI_EXEC_PATH, 0500) < 0) {
        LOGE("%s: Failed to chmod: %s", BOOT_UI_EXEC_PATH, strerror(errno));
        return false;
    }

    start_daemon();

    std::vector<std::string> argv{ BOOT_UI_EXEC_PATH, BOOT_UI_ZIP_PATH };
    int ret = util::run_command(argv[0], argv, {}, {}, {});
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

static void redirect_stdio_null()
{
    static constexpr char name[] = "/dev/__null__";

    if (mknod(name, S_IFCHR | 0600, makedev(1, 3)) == 0) {
        // O_CLOEXEC should not be used
        int fd = open(name, O_RDWR);
        unlink(name);
        if (fd >= 0) {
            dup2(fd, 0);
            dup2(fd, 1);
            dup2(fd, 2);
            if (fd > 2) {
                close(fd);
            }
        }
    }
}

static bool critical_failure()
{
    return emergency_reboot();
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
    (void) util::chown("/cache", "system", "cache", 0);
    (void) util::chown("/data", "system", "system", 0);

    // Redirect std{in,out,err} to /dev/null
    redirect_stdio_null();

    // Log to kmsg
    log::set_logger(std::make_shared<log::KmsgLogger>(true));
    if (klogctl(KLOG_CONSOLE_LEVEL, nullptr, 7) < 0) {
        LOGE("Failed to set loglevel: %s", strerror(errno));
    }

    LOGV("Booting up with version %s (%s)",
         version(), git_version());

    auto contents = util::file_read_all(DEVICE_JSON_PATH);
    if (!contents) {
        LOGE("%s: Failed to read file: %s", DEVICE_JSON_PATH,
             contents.error().message().c_str());
        critical_failure();
        return EXIT_FAILURE;
    }

    // Start probing for devices so we have somewhere to write logs for
    // critical_failure()
    UeventThread uevent_thread;
    uevent_thread.start();

    Device device;
    JsonError error;

    if (!device_from_json(contents.value(), device, error)) {
        LOGE("%s: Failed to load device definition", DEVICE_JSON_PATH);
        critical_failure();
        return EXIT_FAILURE;
    } else if (device.validate()) {
        LOGE("%s: Device definition validation failed", DEVICE_JSON_PATH);
        critical_failure();
        return EXIT_FAILURE;
    }

    // Symlink by-name directory to /dev/block/by-name (ugh... ASUS)
    symlink_base_dir(device);

    add_props_to_dbp_prop();

    // initialize properties
    properties_setup();

    std::string fstab(find_fstab());

    LOGV("fstab file: %s", fstab.c_str());

    if (access(fstab.c_str(), R_OK) < 0) {
        LOGW("%s: Failed to access file: %s", fstab.c_str(), strerror(errno));
        LOGW("Continuing anyway...");
        fstab = "/fstab.MBTOOL_DUMMY_DO_NOT_USE";
        (void) util::create_empty_file(fstab);
    }

    // Get ROM ID from /romid
    std::string rom_id = get_rom_id();
    std::shared_ptr<Rom> rom = Roms::create_rom(rom_id);
    if (!rom) {
        LOGE("Unknown ROM ID: %s", rom_id.c_str());
        critical_failure();
        return EXIT_FAILURE;
    }

    LOGV("ROM ID is: %s", rom_id.c_str());

    // Mount system, cache, and external SD from fstab file
    MountFlags flags =
            MountFlag::RewriteFstab
            | MountFlag::MountSystem
            | MountFlag::MountCache
            | MountFlag::MountData
            | MountFlag::MountExternalSd;
    if (!mount_fstab(fstab.c_str(), rom, device, flags,
                     uevent_thread.device_handler())) {
        LOGE("Failed to mount fstab");
        critical_failure();
        return EXIT_FAILURE;
    }

    LOGV("Successfully mounted fstab");

    if (!launch_boot_menu()) {
        LOGE("Failed to run boot menu");
        // Continue anyway since boot menu might not run on every device
    }

    // Mount selinuxfs
    selinux_mount();
    // Load pre-boot policy
    patch_sepolicy(util::SELINUX_DEFAULT_POLICY_FILE, util::SELINUX_LOAD_FILE,
                   SELinuxPatch::PreBoot);

    // Mount ROM (bind mount directory or mount images, etc.)
    if (!mount_rom(rom)) {
        LOGE("Failed to mount ROM directories and images");
        critical_failure();
        return EXIT_FAILURE;
    }

    std::string config_path(rom->config_path());
    RomConfig config;
    if (!config.load_file(config_path)) {
        LOGW("%s: Failed to load config for ROM %s",
             config_path.c_str(), rom->id.c_str());
    }

    LOGD("Enable appsync: %d", config.indiv_app_sharing);

    // Make runtime ramdisk modifications
    if (access(FILE_CONTEXTS, R_OK) == 0) {
        fix_file_contexts(FILE_CONTEXTS);
    }
    if (access(FILE_CONTEXTS_BIN, R_OK) == 0) {
        fix_binary_file_contexts(FILE_CONTEXTS_BIN);
    }
    write_fstab_hack(fstab.c_str());
    add_mbtool_services(config.indiv_app_sharing);
    strip_manual_mounts();

    // Disable installd on Android 7.0+
    if (config.indiv_app_sharing) {
        disable_installd();
    }

    // Data modifications
    create_layout_version();

    // Disable spota
    disable_spota();

    // Patch SELinux policy
    struct stat sb;
    if (stat(util::SELINUX_DEFAULT_POLICY_FILE, &sb) == 0) {
        if (!patch_sepolicy(util::SELINUX_DEFAULT_POLICY_FILE,
                            util::SELINUX_DEFAULT_POLICY_FILE,
                            SELinuxPatch::Main)) {
            LOGW("%s: Failed to patch policy",
                 util::SELINUX_DEFAULT_POLICY_FILE);
            critical_failure();
            return EXIT_FAILURE;
        }
    }

    // Kill uevent thread and close uevent socket
    uevent_thread.stop();

    // Kill properties service and clean up
    properties_cleanup();

    // Hack to work around issue where Magisk unconditionally unmounts /system
    // https://github.com/topjohnwu/Magisk/pull/387
    if (util::file_find_one_of("/init.orig", {"MagiskPolicy v16"})) {
        mount("/system", "/system", "", MS_BIND, "");
    }

    // Remove mbtool init symlink and restore original binary
    unlink("/init");
    rename("/init.orig", "/init");

    // Unmount partitions
    selinux_unmount();
    umount("/dev/pts");
    // Detach because we still hold a kmsg fd open
    umount2("/dev", MNT_DETACH);
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
    critical_failure();
    return EXIT_FAILURE;
}

}
