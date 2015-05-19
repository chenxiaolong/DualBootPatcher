/*
 * Copyright (C) 2014  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mount_fstab.h"

#include <algorithm>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <getopt.h>
#include <libgen.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/xattr.h>
#include <unistd.h>

#include "reboot.h"
#include "romconfig.h"
#include "roms.h"
#include "sepolpatch.h"
#include "util/cmdline.h"
#include "util/directory.h"
#include "util/file.h"
#include "util/finally.h"
#include "util/fstab.h"
#include "util/logging.h"
#include "util/loopdev.h"
#include "util/mount.h"
#include "util/path.h"
#include "util/properties.h"
#include "util/string.h"


#define FORCE_SELINUX_PERMISSIVE 0


namespace mb
{

typedef std::unique_ptr<std::FILE, int (*)(std::FILE *)> file_ptr;

static bool create_dir_and_mount(const std::vector<util::fstab_rec *> recs,
                                 const std::vector<util::fstab_rec *> flags,
                                 const std::string &mount_point)
{
    if (recs.empty() || flags.empty()) {
        return false;
    }

    LOGD("%zu fstab entries for %s", recs.size(), recs[0]->mount_point.c_str());

    // Copy permissions of the original mountpoint directory if it exists
    struct stat sb;
    mode_t perms;

    if (stat(recs[0]->mount_point.c_str(), &sb) == 0) {
        perms = sb.st_mode & 0xfff;
    } else {
        LOGW("%s found in fstab, but %s does not exist",
             recs[0]->mount_point.c_str(), recs[0]->mount_point.c_str());
        perms = 0771;
    }

    if (stat(mount_point.c_str(), &sb) == 0) {
        if (chmod(mount_point.c_str(), perms) < 0) {
            LOGE("Failed to chmod %s: %s",
                 mount_point.c_str(), strerror(errno));
            return false;
        }
    } else {
        if (mkdir(mount_point.c_str(), perms) < 0) {
            LOGE("Failed to create %s: %s",
                 mount_point.c_str(), strerror(errno));
            return false;
        }
    }

    // Try mounting each until we find one that works
    for (util::fstab_rec *rec : recs) {
        LOGD("Attempting to mount %s (%s) at %s",
             rec->blk_device.c_str(), rec->fs_type.c_str(), mount_point.c_str());

        // Find flags with a matching filesystem
        auto it = std::find_if(flags.begin(), flags.end(),
                [&rec](const util::fstab_rec *frec) {
                    return frec->fs_type == rec->fs_type;
                });
        if (it == flags.end()) {
            LOGE("Failed to find matching fstab record in flags list");
            continue;
        }

        // Try mounting
        int ret = mount(rec->blk_device.c_str(),
                        mount_point.c_str(),
                        rec->fs_type.c_str(),
                        (*it)->flags,
                        (*it)->fs_options.c_str());
        if (ret < 0) {
            LOGE("Failed to mount %s (%s) at %s: %s",
                 rec->blk_device.c_str(), rec->fs_type.c_str(),
                 mount_point.c_str(), strerror(errno));
            continue;
        } else {
            LOGE("Successfully mounted %s (%s) at %s",
                 rec->blk_device.c_str(), rec->fs_type.c_str(),
                 mount_point.c_str());
            return true;
        }
    }

    return false;
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

bool mount_fstab(const std::string &fstab_path)
{
    bool ret = true;

    std::vector<util::fstab_rec> fstab;
    std::vector<util::fstab_rec *> recs_system;
    std::vector<util::fstab_rec *> recs_cache;
    std::vector<util::fstab_rec *> recs_data;
    std::vector<util::fstab_rec *> flags_system;
    std::vector<util::fstab_rec *> flags_cache;
    std::vector<util::fstab_rec *> flags_data;
    std::string target_system;
    std::string target_cache;
    std::string target_data;
    std::string path_fstab_gen;
    std::string path_completed;
    std::string path_failed;
    std::string base_name;
    std::string dir_name;
    struct stat st;
    std::shared_ptr<Rom> rom;
    std::string rom_id;

    Roms roms;
    roms.add_builtin();

    base_name = util::base_name(fstab_path);
    dir_name = util::dir_name(fstab_path);

    path_fstab_gen += dir_name;
    path_fstab_gen += "/.";
    path_fstab_gen += base_name;
    path_fstab_gen += ".gen";
    path_completed += dir_name;
    path_completed += "/.";
    path_completed += base_name;
    path_completed += ".completed";
    path_failed += dir_name;
    path_failed += "/.";
    path_failed += base_name;
    path_failed += ".failed";

    auto on_finish = util::finally([&] {
        if (ret) {
            util::create_empty_file(path_completed);
            LOGI("Successfully mounted partitions");
        } else {
            util::create_empty_file(path_failed);
        }
    });

    // This is a oneshot operation
    if (stat(path_completed.c_str(), &st) == 0) {
        LOGV("Filesystems already successfully mounted");
        return true;
    }

    if (stat(path_failed.c_str(), &st) == 0) {
        LOGE("Failed to mount partitions ealier. No further attempts will be made");
        return false;
    }

    // Remount rootfs as read-write so a new fstab file can be written
    if (mount("", "/", "", MS_REMOUNT, "") < 0) {
        LOGE("Failed to remount rootfs as rw: %s", strerror(errno));
    }

    // Read original fstab
    fstab = util::read_fstab(fstab_path);
    if (fstab.empty()) {
        LOGE("Failed to read %s", fstab_path.c_str());
        return false;
    }

    // Generate new fstab without /system, /cache, or /data entries
    file_ptr out(std::fopen(path_fstab_gen.c_str(), "wb"), std::fclose);
    if (!out) {
        LOGE("Failed to open %s for writing: %s",
             path_fstab_gen.c_str(), strerror(errno));
        return false;
    }

    for (util::fstab_rec &rec : fstab) {
        if (rec.mount_point == "/system") {
            recs_system.push_back(&rec);
        } else if (rec.mount_point == "/cache") {
            recs_cache.push_back(&rec);
        } else if (rec.mount_point == "/data") {
            recs_data.push_back(&rec);
        } else {
            std::fprintf(out.get(), "%s\n", rec.orig_line.c_str());
        }
    }

    out.reset();

    // /system and /data are always in the fstab. The patcher should create
    // an entry for /cache for the ROMs that mount it manually in one of the
    // init scripts
    if (recs_system.empty() || recs_cache.empty() || recs_data.empty()) {
        LOGE("fstab does not contain all of /system, /cache, and /data!");
        return false;
    }

    // Mount raw partitions to /raw/*
    if (!util::kernel_cmdline_get_option("romid", &rom_id)
            && !util::file_first_line("/romid", &rom_id)) {
        LOGE("Failed to determine ROM ID");
        return false;
    }

    if (Roms::is_named_rom(rom_id)) {
        rom = Roms::create_named_rom(rom_id);
    } else {
        rom = roms.find_by_id(rom_id);
        if (!rom) {
            LOGE("Unknown ROM ID: %s", rom_id.c_str());
            return false;
        }
    }

    LOGD("ROM ID is: %s", rom_id.c_str());

    // Set property for the Android app to use
    if (!util::set_property("ro.multiboot.romid", rom_id)) {
        LOGE("Failed to set 'ro.multiboot.romid' to '%s'", rom_id.c_str());
    }

    // Because of how Android deals with partitions, if, say, the source path
    // for the /system bind mount resides on /cache, then the cache partition
    // must be mounted with the system partition's flags. In this future, this
    // may be avoided by mounting every partition with some more liberal flags,
    // since the current setup does not allow two bind mounted locations to
    // reside on the same partition.

    if (util::starts_with(rom->system_path, "/cache")) {
        flags_system = recs_cache;
    } else {
        flags_system = recs_system;
    }

    if (util::starts_with(rom->cache_path, "/system")) {
        flags_cache = recs_system;
    } else {
        flags_cache = recs_cache;
    }

    flags_data = recs_data;

    if (mkdir("/raw", 0755) < 0) {
        LOGE("Failed to create /raw");
        return false;
    }

    if (!create_dir_and_mount(recs_system, flags_system, "/raw/system")) {
        LOGE("Failed to mount /raw/system");
        return false;
    }
    if (!create_dir_and_mount(recs_cache, flags_cache, "/raw/cache")) {
        LOGE("Failed to mount /raw/cache");
        return false;
    }
    if (!create_dir_and_mount(recs_data, flags_data, "/raw/data")) {
        LOGE("Failed to mount /raw/data");
        return false;
    }

    // Make paths use /raw/...
    if (rom->system_path.empty()
            || rom->cache_path.empty()
            || rom->data_path.empty()) {
        LOGE("Invalid or empty paths");
        return false;
    }

    target_system += "/raw";
    target_system += rom->system_path;
    target_cache += "/raw";
    target_cache += rom->cache_path;
    target_data += "/raw";
    target_data += rom->data_path;

    if (!util::bind_mount(target_system, 0771, "/system", 0771)) {
        return false;
    }

    if (!util::bind_mount(target_cache, 0771, "/cache", 0771)) {
        return false;
    }

    if (!util::bind_mount(target_data, 0771, "/data", 0771)) {
        return false;
    }

    // Bind mount internal SD directory
    if (!util::bind_mount("/raw/data/media", 0771, "/data/media", 0771)) {
        return false;
    }

    // Prevent installd from dying because it can't unmount /data/media for
    // multi-user migration. Since <= 4.2 devices aren't supported anyway,
    // we'll bypass this.
    file_ptr fp(std::fopen("/data/.layout_version", "wb"), std::fclose);
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
        LOGE("Failed to open /data/.layout_version to disable migration");
    }

    static std::string context("u:object_r:install_data_file:s0");
    if (lsetxattr("/data/.layout_version", "security.selinux",
                  context.c_str(), context.size() + 1, 0) < 0) {
        LOGE("%s: Failed to set SELinux context: %s",
             "/data/.layout_version", strerror(errno));
    }


    // Global app sharing
    std::string config_path("/data/media/0/MultiBoot/");
    config_path += rom->id;
    config_path += "/config.json";

    RomConfig config;
    if (config.load_file(config_path)) {
        if (config.indiv_app_sharing && (config.global_app_sharing
                || config.global_paid_app_sharing)) {
            LOGW("Both individual and global sharing are enabled");
            LOGW("Global sharing settings will be ignored");
        } else {
            if (config.global_app_sharing || config.global_paid_app_sharing) {
                if (!util::bind_mount("/raw/data/app-lib", 0771,
                                      "/data/app-lib", 0771)) {
                    return false;
                }
            }
            if (config.global_app_sharing) {
                if (!util::bind_mount("/raw/data/app", 0771,
                                      "/data/app", 0771)) {
                    return false;
                }
            }
            if (config.global_paid_app_sharing) {
                if (!util::bind_mount("/raw/data/app-asec", 0771,
                                      "/data/app-asec", 0771)) {
                    return false;
                }
            }
        }
    }

    return true;
}

static void mount_fstab_usage(int error)
{
    FILE *stream = error ? stderr : stdout;

    fprintf(stream,
            "Usage: mount_fstab [fstab file]\n\n"
            "This takes only one argument, the path to the fstab file. If the\n"
            "mounting succeeds, the generated \"/.<orig fstab>.gen\" file should\n"
            "be passed to the Android init's mount_all command.\n");
}

int mount_fstab_main(int argc, char *argv[])
{
    int opt;

    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int long_index = 0;

    while ((opt = getopt_long(argc, argv, "h", long_options, &long_index)) != -1) {
        switch (opt) {
        case 'h':
            mount_fstab_usage(0);
            return EXIT_SUCCESS;

        default:
            mount_fstab_usage(1);
            return EXIT_FAILURE;
        }
    }

    // We only expect one argument
    if (argc - optind != 1) {
        mount_fstab_usage(1);
        return EXIT_FAILURE;
    }

    // Use the kernel log since logcat hasn't run yet
    util::log_set_logger(std::make_shared<util::KmsgLogger>());

    // Patch SELinux policy
    if (!patch_loaded_sepolicy()) {
        LOGE("Failed to patch loaded SELinux policy. Continuing anyway");
    } else {
        LOGV("SELinux policy patching completed");
    }

#if FORCE_SELINUX_PERMISSIVE
    int fd = open("/sys/fs/selinux/enforce", O_RDWR);
    if (fd > 0) {
        write(fd, "0", 1);
        close(fd);
    }
#endif

    if (!mount_fstab(argv[optind])) {
        LOGE("Failed to mount filesystems. Rebooting into recovery");
        reboot_directly("recovery");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

}
