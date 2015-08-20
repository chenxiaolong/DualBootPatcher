/*
 * Copyright (C) 2014-2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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
#include <dirent.h>
#include <fcntl.h>
#include <getopt.h>
#include <libgen.h>
#include <signal.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <unistd.h>

#include "reboot.h"
#include "romconfig.h"
#include "roms.h"
#include "sepolpatch.h"
#include "util/cmdline.h"
#include "util/copy.h"
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




namespace mb
{

typedef std::unique_ptr<std::FILE, int (*)(std::FILE *)> file_ptr;

static std::shared_ptr<Rom> determine_rom()
{
    std::shared_ptr<Rom> rom;
    std::string rom_id;

    Roms roms;
    roms.add_builtin();

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
        if (rom) {
            LOGD("ROM ID is: %s", rom_id.c_str());
        } else {
            LOGE("Unknown ROM ID: %s", rom_id.c_str());
        }
    }

    return rom;
}

/*!
 * \brief Try mounting each entry in a list of fstab entry until one works.
 *
 * \param recs List of fstab entries for the given mount point
 * \param mount_point Target mount point
 *
 * \return Whether some fstab entry was successfully mounted at the mount point
 */
static bool create_dir_and_mount(const std::vector<util::fstab_rec *> &recs,
                                 const std::string &mount_point)
{
    if (recs.empty()) {
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

        // Try mounting
        int ret = mount(rec->blk_device.c_str(),
                        mount_point.c_str(),
                        rec->fs_type.c_str(),
                        rec->flags,
                        rec->fs_options.c_str());
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

/*!
 * \note This really is a horrible app sharing method and should be removed at
 * some point.
 *
 * If global app sharing is enabled:
 *   1. Bind mount /raw/data/app-lib  -> /data/app-lib
 *   2. Bind mount /raw/data/app      -> /data/app
 *   3. Bind mount /raw/data/app-asec -> /data/app-asec
 *
 * If individual app sharing is enabled, it takes precedence over global app
 * sharing.
 */
static bool apply_global_app_sharing(const std::shared_ptr<Rom> &rom)
{
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

/*!
 * \brief Get list of generic /cache fstab entries for ROMs that mount the
 *        partition manually
 */
static std::vector<util::fstab_rec> generic_fstab_cache_entries()
{
    return std::vector<util::fstab_rec>{
        {
            .blk_device = "/dev/block/platform/msm_sdcc.1/by-name/cache",
            .mount_point = "/cache",
            .fs_type = "ext4",
            .flags = MS_NOSUID | MS_NODEV,
            .fs_options = "barrier=1",
            .vold_args = "wait,check",
            .orig_line = std::string()
        }
        // Add more as necessary...
    };
}

/*!
 * \brief Write new fstab file
 */
static bool write_generated_fstab(const std::vector<util::fstab_rec *> &recs,
                                  const std::string &path, mode_t mode)
{
    // Generate new fstab without /system, /cache, or /data entries
    file_ptr out(std::fopen(path.c_str(), "wb"), std::fclose);
    if (!out) {
        LOGE("Failed to open %s for writing: %s",
             path.c_str(), strerror(errno));
        return false;
    }

    fchmod(fileno(out.get()), mode & 0777);

    for (util::fstab_rec *rec : recs) {
        std::fprintf(out.get(), "%s\n", rec->orig_line.c_str());
    }

    return true;
}

/*!
 * \brief Mount specified /system, /cache, and /data fstab entries
 */
static bool mount_fstab_entries(const std::vector<util::fstab_rec *> &system_recs,
                                const std::vector<util::fstab_rec *> &cache_recs,
                                const std::vector<util::fstab_rec *> &data_recs,
                                const std::shared_ptr<Rom> &rom)
{
    if (mkdir("/raw", 0755) < 0) {
        LOGE("Failed to create /raw: %s", strerror(errno));
        return false;
    }

    // Mount raw partitions
    if (!create_dir_and_mount(system_recs, "/raw/system")) {
        LOGE("Failed to mount /raw/system");
        return false;
    }
    if (!create_dir_and_mount(cache_recs, "/raw/cache")) {
        LOGE("Failed to mount /raw/cache");
        return false;
    }
    if (!create_dir_and_mount(data_recs, "/raw/data")) {
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

    std::string target_system("/raw");
    std::string target_cache("/raw");
    std::string target_data("/raw");
    target_system += rom->system_path;
    target_cache += rom->cache_path;
    target_data += rom->data_path;

    // Bind mount proper /system, /cache, and /data directories
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

    return true;
}

bool mount_fstab(const std::string &fstab_path, bool overwrite_fstab)
{
    std::vector<util::fstab_rec> fstab;
    std::vector<util::fstab_rec> fstab_cache;
    std::vector<util::fstab_rec *> recs_gen;
    std::vector<util::fstab_rec *> recs_system;
    std::vector<util::fstab_rec *> recs_cache;
    std::vector<util::fstab_rec *> recs_data;
    std::string path_fstab_gen;
    std::string path_completed;
    std::string base_name;
    std::string dir_name;
    struct stat st;
    std::shared_ptr<Rom> rom;
    std::string rom_id;

    rom = determine_rom();
    if (!rom) {
        return false;
    }

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

    // This is a oneshot operation
    if (stat(path_completed.c_str(), &st) == 0) {
        util::create_empty_file(path_completed);
        LOGV("Filesystems already successfully mounted");
        return true;
    }

    // Read original fstab
    fstab = util::read_fstab(fstab_path);
    if (fstab.empty()) {
        LOGE("Failed to read %s", fstab_path.c_str());
        return false;
    }

    if (stat(fstab_path.c_str(), &st) < 0) {
        LOGE("Failed to stat fstab %s: %s",
             fstab_path.c_str(), strerror(errno));
        return false;
    }

    // Generate new fstab without /system, /cache, or /data entries
    for (util::fstab_rec &rec : fstab) {
        if (rec.mount_point == "/system") {
            recs_system.push_back(&rec);
        } else if (rec.mount_point == "/cache") {
            recs_cache.push_back(&rec);
        } else if (rec.mount_point == "/data") {
            recs_data.push_back(&rec);
        } else {
            recs_gen.push_back(&rec);
        }
    }

    if (!write_generated_fstab(recs_gen, path_fstab_gen, st.st_mode)) {
        return false;
    }

    if (overwrite_fstab) {
        // For backwards compatibility, keep the old generated fstab as the
        // init.*.rc file will refer to it under the generated name
        util::copy_contents(path_fstab_gen, fstab_path);
        //unlink(fstab_path.c_str());
        //rename(path_fstab_gen.c_str(), fstab_path.c_str());
    }

    // Some ROMs mount /cache in one of the init.*.rc files. If that's the case
    // try using a generic fstab entry for it.
    if (recs_cache.empty()) {
        fstab_cache = generic_fstab_cache_entries();
        for (util::fstab_rec &rec : fstab_cache) {
            recs_cache.push_back(&rec);
        }
    }

    // /system and /data are always in the fstab. The patcher should create
    // an entry for /cache for the ROMs that mount it manually in one of the
    // init scripts
    if (recs_system.empty() || recs_cache.empty() || recs_data.empty()) {
        LOGE("fstab does not contain all of /system, /cache, and /data!");
        return false;
    }

    if (util::starts_with(rom->system_path, "/cache")) {
        for (util::fstab_rec *rec : recs_cache) {
            rec->flags &= ~MS_NOSUID;
        }
    } else if (util::starts_with(rom->system_path, "/data")) {
        for (util::fstab_rec *rec : recs_data) {
            rec->flags &= ~MS_NOSUID;
        }
    }
    if (util::starts_with(rom->cache_path, "/system")) {
        for (util::fstab_rec *rec : recs_system) {
            rec->flags &= ~MS_RDONLY;
        }
    }

    if (!mount_fstab_entries(recs_system, recs_cache, recs_data, rom)) {
        return false;
    }

    if (!apply_global_app_sharing(rom)) {
        return false;
    }

    // Set property for the Android app to use
    if (!util::set_property("ro.multiboot.romid", rom_id)) {
        LOGE("Failed to set 'ro.multiboot.romid' to '%s'", rom_id.c_str());
    }

    util::create_empty_file(path_completed);
    LOGI("Successfully mounted partitions");
    return true;
}

int mount_fstab_main(int argc, char *argv[])
{
    (void) argc;
    (void) argv;

    printf("mount_fstab has been deprecated and removed. "
           "The initwrapper should be used instead\n.");
    return EXIT_SUCCESS;
}

}
