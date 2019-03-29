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

#include "main/boot.h"

#include <algorithm>
#include <memory>

#include <cerrno>
#include <cstdlib>
#include <cstring>

#include <fcntl.h>
#include <getopt.h>
#include <signal.h>
#include <sys/klog.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <unistd.h>

#include "mbcommon/finally.h"
#include "mbcommon/string.h"
#include "mbcommon/type_traits.h"
#include "mbcommon/version.h"
#include "mblog/logging.h"
#include "mbutil/chown.h"
#include "mbutil/cmdline.h"
#include "mbutil/command.h"
#include "mbutil/delete.h"
#include "mbutil/directory.h"
#include "mbutil/file.h"
#include "mbutil/mount.h"
#include "mbutil/path.h"
#include "mbutil/properties.h"
#include "mbutil/selinux.h"
#include "mbutil/string.h"

#include "util/android_api.h"
#include "util/emergency.h"
#include "util/kmsg.h"
#include "util/multiboot.h"
#include "util/roms.h"
#include "util/sepolpatch.h"
#include "util/signature.h"

#if defined(__i386__) || defined(__arm__)
#define PCRE_PATH               "/system/lib/libpcre.so"
#elif defined(__x86_64__) || defined(__aarch64__)
#define PCRE_PATH               "/system/lib64/libpcre.so"
#else
#error Unknown PCRE path for architecture
#endif

#define LOG_TAG "mbtool/main/init"

namespace mb
{

using ScopedFILE = std::unique_ptr<FILE, TypeFn<fclose>>;

static void boot_usage(FILE *stream)
{
    fprintf(stream,
            "Usage: boot"
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

    ScopedFILE fp_old(fopen(path, "rbe"));
    if (!fp_old) {
        if (errno == ENOENT) {
            return true;
        } else {
            LOGE("%s: Failed to open for reading: %s",
                 path, strerror(errno));
            return false;
        }
    }

    ScopedFILE fp_new(fopen(new_path.c_str(), "wbe"));
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
        if (starts_with(line, INTERNAL_STORAGE_ROOT "(")
                && !strstr(line, "<<none>>")) {
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
            INTERNAL_STORAGE_ROOT                 " <<none>>\n"
            INTERNAL_STORAGE_ROOT "/[0-9]+(/.*)?" " <<none>>\n"
            "/raw(/.*)?"                          " <<none>>\n"
            "/data/multiboot(/.*)?"               " <<none>>\n"
            "/cache/multiboot(/.*)?"              " <<none>>\n"
            "/system/multiboot(/.*)?"             " <<none>>\n";
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
    if (auto r = verify_signature("/sbin/file-contexts-tool", nullptr); !r) {
        LOGE("%s: Failed to verify signature: %s",
             "/sbin/file-contexts-tool", r.error().message().c_str());
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

static bool create_layout_version()
{
    // Prevent installd from dying because it can't unmount /data/media for
    // multi-user migration. Since <= 4.2 devices aren't supported anyway,
    // we'll bypass this.
    ScopedFILE fp(fopen("/data/.layout_version", "wbe"));
    if (fp) {
        const char *layout_version;
        if (get_sdk_version(SdkVersionSource::BuildProp) >= 21) {
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

static bool disable_spota()
{
    static const char *spota_dir = "/data/security/spota";

    auto props = util::property_file_get_all(BUILD_PROP_PATH);

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

int boot_main(int argc, char *argv[])
{
    static const option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    int long_index = 0;

    while ((opt = getopt_long(argc, argv, "h", long_options, &long_index)) != -1) {
        switch (opt) {
        case 'h':
            boot_usage(stdout);
            return EXIT_SUCCESS;

        default:
            boot_usage(stderr);
            return EXIT_FAILURE;
        }
    }

    // There should be no other arguments
    if (argc - optind != 0) {
        boot_usage(stderr);
        return EXIT_FAILURE;
    }

    // Create mount points
    mkdir("/system", 0755);
    mkdir("/cache", 0770);
    mkdir("/data", 0771);
    (void) util::chown("/cache", "system", "cache", 0);
    (void) util::chown("/data", "system", "system", 0);

    set_kmsg_logging();

    LOGV("Booting up with version %s (%s)", version(), git_version());

    // Get ROM ID from /romid
    std::string rom_id = get_rom_id();
    std::shared_ptr<Rom> rom = Roms::create_rom(rom_id);
    if (!rom) {
        LOGE("Unknown ROM ID: %s", rom_id.c_str());
        emergency_reboot();
    }

    LOGV("ROM ID is: %s", rom_id.c_str());

    // Mount selinuxfs
    selinux_mount();
    // Load pre-boot policy
    patch_sepolicy(util::SELINUX_DEFAULT_POLICY_FILE, util::SELINUX_LOAD_FILE,
                   SELinuxPatch::PreBoot);

    // Make runtime ramdisk modifications
    if (access(FILE_CONTEXTS, R_OK) == 0) {
        fix_file_contexts(FILE_CONTEXTS);
    }
    if (access(FILE_CONTEXTS_BIN, R_OK) == 0) {
        fix_binary_file_contexts(FILE_CONTEXTS_BIN);
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
            emergency_reboot();
        }
    }

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
    emergency_reboot();
}

}
