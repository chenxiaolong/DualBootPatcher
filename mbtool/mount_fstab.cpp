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
#include <fnmatch.h>
#include <getopt.h>
#include <libgen.h>
#include <pwd.h>
#include <signal.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "reboot.h"
#include "romconfig.h"
#include "roms.h"
#include "sepolpatch.h"
#include "initwrapper/devices.h"
#include "util/cmdline.h"
#include "util/command.h"
#include "util/copy.h"
#include "util/directory.h"
#include "util/file.h"
#include "util/finally.h"
#include "util/fstab.h"
#include "util/logging.h"
#include "util/mount.h"
#include "util/path.h"
#include "util/properties.h"
#include "util/string.h"


#define EXTSD_MOUNT_POINT "/raw/extsd"
#define IMAGES_MOUNT_POINT "/raw/images"


namespace mb
{

typedef std::unique_ptr<std::FILE, int (*)(std::FILE *)> file_ptr;

static std::shared_ptr<Rom> determine_rom()
{
    std::shared_ptr<Rom> rom;
    std::string rom_id;

    // Mount raw partitions to /raw/*
    if (!util::kernel_cmdline_get_option("romid", &rom_id)
            && !util::file_first_line("/romid", &rom_id)) {
        LOGE("Failed to determine ROM ID");
        return false;
    }

    rom = Roms::create_rom(rom_id);
    if (rom) {
        LOGD("ROM ID is: %s", rom_id.c_str());
    } else {
        LOGE("Unknown ROM ID: %s", rom_id.c_str());
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

    LOGD("Tried all %zu fstab entries for %s, but couldn't mount any",
         recs.size(), mount_point.c_str());

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
 * \brief Get list of generic /system fstab entries for ROMs that mount the
 *        partition manually
 */
static std::vector<util::fstab_rec> generic_fstab_system_entries()
{
    return {
        {
            .blk_device = "/dev/block/platform/15570000.ufs/by-name/SYSTEM",
            .mount_point = "/system",
            .fs_type = "ext4",
            .flags = MS_RDONLY | MS_NOATIME | MS_NODIRATIME,
            .fs_options = "noauto_da_alloc,nodiscard,data=ordered,errors=panic",
            .vold_args = "wait,check",
            .orig_line = std::string()
        },
        {
            .blk_device = "/dev/block/platform/15570000.ufs/by-name/SYSTEM",
            .mount_point = "/system",
            .fs_type = "f2fs",
            .flags = MS_RDONLY | MS_NOATIME | MS_NODIRATIME,
            .fs_options = "background_gc=off,nodiscard",
            .vold_args = "wait,check",
            .orig_line = std::string()
        }
        // Add more as necessary
    };
}

/*!
 * \brief Get list of generic /cache fstab entries for ROMs that mount the
 *        partition manually
 */
static std::vector<util::fstab_rec> generic_fstab_cache_entries()
{
    return {
        {
            .blk_device = "/dev/block/platform/msm_sdcc.1/by-name/cache",
            .mount_point = "/cache",
            .fs_type = "ext4",
            .flags = MS_NOSUID | MS_NODEV,
            .fs_options = "barrier=1",
            .vold_args = "wait,check",
            .orig_line = std::string()
        },
        {
            .blk_device = "/dev/block/platform/15570000.ufs/by-name/CACHE",
            .mount_point = "/cache",
            .fs_type = "ext4",
            .flags = MS_NOATIME | MS_NODIRATIME | MS_NOSUID | MS_NODEV,
            .fs_options = "noauto_da_alloc,discard,data=ordered,errors=panic",
            .vold_args = "wait,check",
            .orig_line = std::string()
        },
        {
            .blk_device = "/dev/block/platform/15570000.ufs/by-name/CACHE",
            .mount_point = "/cache",
            .fs_type = "f2fs",
            .flags = MS_NOATIME | MS_NODIRATIME | MS_NOSUID | MS_NODEV,
            .fs_options = "background_gc=on,discard",
            .vold_args = "wait,check",
            .orig_line = std::string()
        }
        // Add more as necessary...
    };
}

/*!
 * \brief Get list of generic /data fstab entries for ROMs that mount the
 *        partition manually
 */
static std::vector<util::fstab_rec> generic_fstab_data_entries()
{
    return {
        {
            .blk_device = "/dev/block/platform/15570000.ufs/by-name/USERDATA",
            .mount_point = "/data",
            .fs_type = "ext4",
            .flags = MS_NOATIME | MS_NODIRATIME | MS_NOSUID | MS_NODEV,
            .fs_options = "noauto_da_alloc,discard,data=ordered,errors=panic",
            .vold_args = "wait,check",
            .orig_line = std::string()
        },
        {
            .blk_device = "/dev/block/platform/15570000.ufs/by-name/USERDATA",
            .mount_point = "/data",
            .fs_type = "f2fs",
            .flags = MS_NOATIME | MS_NODIRATIME | MS_NOSUID | MS_NODEV,
            .fs_options = "background_gc=on,discard",
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
                                const std::vector<util::fstab_rec *> &data_recs)
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

    return true;
}

static bool path_matches(const char *path, const char *pattern)
{
    // Vold uses prefix matching if no '*' exists. Otherwise, globbing is used.

    if (strchr(pattern, '*')) {
        return fnmatch(pattern, path, 0) == 0;
    } else {
        return util::starts_with(path, pattern);
    }
}

static void dump(const std::string &line, void *data)
{
    (void) data;
    LOGD("Command output: %s", line.c_str());
}

static uid_t get_media_rw_uid()
{
    struct passwd *pw = getpwnam("media_rw");
    if (!pw) {
        return 1023;
    } else {
        return pw->pw_uid;
    }
}

static bool mount_exfat_fuse(const std::string &source,
                             const std::string &target)
{
    uid_t uid = get_media_rw_uid();

    // Run filesystem checks
    //util::run_command_cb({
    //    "/sbin/fsck.exfat",
    //    source
    //}, &dump, nullptr);

    // Mount exfat, matching vold options as much as possible
    int ret = util::run_command_cb({
        "/sbin/mount.exfat",
        "-o",
        util::format(
            "noatime,nodev,nosuid,dirsync,uid=%d,gid=%d,fmask=%o,dmask=%o,%s,%s",
            uid, uid, 0007, 0007, "noexec", "rw"
        ),
        source,
        target
    }, &dump, nullptr);

    if (ret >= 0) {
        LOGD("mount.exfat returned: %d", WEXITSTATUS(ret));
    }

    if (ret < 0) {
        LOGE("Failed to launch /sbin/mount.exfat: %s", strerror(errno));
        return false;
    } else if (WEXITSTATUS(ret) != 0) {
        LOGE("Failed to mount %s (%s) at %s",
             source.c_str(), "fuse-exfat", target.c_str());
        return false;
    } else {
        LOGE("Successfully mounted %s (%s) at %s",
             source.c_str(), "fuse-exfat", target.c_str());
        return true;
    }
}

static bool mount_exfat_kernel(const std::string &source,
                               const std::string &target)
{
    int ret = mount(source.c_str(), target.c_str(), "exfat", 0, "");
    if (ret < 0) {
        LOGE("Failed to mount %s (%s) at %s: %s",
             source.c_str(), "exfat", target.c_str(), strerror(errno));
        return false;
    } else {
        LOGE("Successfully mounted %s (%s) at %s",
             source.c_str(), "exfat", target.c_str());
        return true;
    }
}

static bool mount_vfat(const std::string &source, const std::string &target)
{
    uid_t uid = get_media_rw_uid();
    std::string args = util::format(
            "utf8,uid=%d,gid=%d,fmask=%o,dmask=%o,shortname=mixed",
            uid, uid, 0007, 0007);
    int flags = MS_NODEV
            | MS_NOSUID
            | MS_DIRSYNC
            | MS_NOEXEC
            // For TouchWiz
            | MS_NOATIME
            | MS_NODIRATIME;

    int ret = mount(source.c_str(), target.c_str(), "vfat", flags, args.c_str());
    if (ret < 0) {
        LOGE("Failed to mount %s (%s) at %s: %s",
             source.c_str(), "vfat", target.c_str(), strerror(errno));
        return false;
    } else {
        LOGE("Successfully mounted %s (%s) at %s",
             source.c_str(), "vfat", target.c_str());
        return true;
    }
}

static bool mount_ext4(const std::string &source, const std::string &target)
{
    int ret = mount(source.c_str(), target.c_str(), "ext4", 0, "");
    if (ret < 0) {
        LOGE("Failed to mount %s (%s) at %s: %s",
             source.c_str(), "ext4", target.c_str(), strerror(errno));
        return false;
    } else {
        LOGE("Successfully mounted %s (%s) at %s",
             source.c_str(), "ext4", target.c_str());
        return true;
    }
}

static bool try_extsd_mount(const std::string &block_dev)
{
    // Vold ignores the fstab fstype field and uses blkid to determine the
    // filesystem. We don't link in blkid, so we'll use a trial and error
    // approach.

    // Ugly hack: CM's vold uses fuse-exfat regardless if the exfat kernel
    // module is available. CM's init binary has the "exfat" and "EXFAT   "
    // due to the linking of libblkid. We'll use that fact to determine whether
    // we're on CM or not.
    if (util::file_find_one_of("/init.orig", { "EXFAT   ", "exfat" })) {
        if (mount_exfat_fuse(block_dev, EXTSD_MOUNT_POINT)) {
            return true;
        }
    } else {
        if (mount_exfat_kernel(block_dev, EXTSD_MOUNT_POINT)) {
            return true;
        }
    }

    if (mount_vfat(block_dev, EXTSD_MOUNT_POINT)) {
        return true;
    }
    if (mount_ext4(block_dev, EXTSD_MOUNT_POINT)) {
        return true;
    }

    return false;
}

/*!
 * \brief Split list of globs from a list of glob strings
 *
 * Splits \a patterns by commas only if the following character is a slash
 */
static std::vector<std::string> split_patterns(const char *patterns)
{
    const char *begin = patterns;
    const char *end;
    std::vector<std::string> result;

    while ((end = strchr(begin, ','))) {
        while (end && *(end + 1) != '/') {
            end = strchr(end + 1, ',');
        }
        if (end) {
            result.emplace_back(begin, end);
            begin = end + 1;
        } else {
            break;
        }
    }

    result.push_back(begin);

    return result;
}

/*!
 * \brief Mount specified external SD fstab entries to /raw/extsd
 *
 * This will *not* do anything if the system wasn't booted using initwrapper.
 * It relies an the sysfs -> block devices map created by initwrapper/devices.cpp
 */
static bool mount_extsd_fstab_entries(const std::vector<util::fstab_rec *> &extsd_recs)
{
    if (extsd_recs.empty()) {
        LOGD("No external SD fstab entries to mount");
        return true;
    }

    LOGD("%zu fstab entries for the external SD", extsd_recs.size());

    if (!util::mkdir_recursive(EXTSD_MOUNT_POINT, 0755)) {
        LOGE("Failed to create %s: %s", EXTSD_MOUNT_POINT, strerror(errno));
        return false;
    }

    // If an actual SD card was found and the mount operation failed, then set
    // to false. This way, the function won't fail if no SD card is inserted.
    bool failed = false;

    auto const *devices_map = get_devices_map();

    for (const util::fstab_rec *rec : extsd_recs) {
        std::vector<std::string> patterns =
                split_patterns(rec->blk_device.c_str());
        for (const std::string &pattern : patterns) {
            bool matched = false;

            for (auto const &pair : *devices_map) {
                if (path_matches(pair.first.c_str(), pattern.c_str())) {
                    matched = true;
                    const std::string &block_dev = pair.second;

                    if (try_extsd_mount(block_dev)) {
                        return true;
                    } else {
                        failed = true;
                    }

                    // Keep trying ...
                }
            }

            if (!matched) {
                LOGE("Failed to find block device corresponding to %s",
                     pattern.c_str());
            }
        }
    }

    return !failed;
}

static bool mount_image(const std::string &image,
                        const std::string &mount_point,
                        mode_t perms)
{
    if (!util::mkdir_recursive(mount_point, perms)) {
        LOGE("Failed to create directory %s: %s",
             mount_point.c_str(), strerror(errno));
        return false;
    }

    // Our image files are always ext4 images
    if (!util::mount(image.c_str(), mount_point.c_str(), "ext4", 0, "")) {
        LOGE("Failed to mount %s: %s", image.c_str(), strerror(errno));
        return false;
    }

    return true;
}

static bool mount_rom(const std::shared_ptr<Rom> &rom)
{
    std::string target_system = rom->full_system_path();
    std::string target_cache = rom->full_cache_path();
    std::string target_data = rom->full_data_path();

    if (target_system.empty() || target_cache.empty() || target_data.empty()) {
        LOGE("Could not determine full path for system, cache, and data");
        LOGE("System: %s", target_system.c_str());
        LOGE("Cache: %s", target_cache.c_str());
        LOGE("Data: %s", target_data.c_str());
        return false;
    }

    if (rom->system_is_image) {
        if (!mount_image(target_system, "/system", 0771)) {
            return false;
        }
    } else {
        if (!util::bind_mount(target_system, 0771, "/system", 0771)) {
            return false;
        }
    }
    if (rom->cache_is_image) {
        if (!mount_image(target_cache, "/cache", 0771)) {
            return false;
        }
    } else {
        if (!util::bind_mount(target_cache, 0771, "/cache", 0771)) {
            return false;
        }
    }
    if (rom->data_is_image) {
        if (!mount_image(target_data, "/data", 0771)) {
            return false;
        }
    } else {
        if (!util::bind_mount(target_data, 0771, "/data", 0771)) {
            return false;
        }
    }

    // Bind mount internal SD directory
    if (!util::bind_mount("/raw/data/media", 0771, "/data/media", 0771)) {
        return false;
    }

    return true;
}

/*!
 * \brief Mount all system image files to /raw/images/[ROM ID]
 */
static bool mount_all_system_images()
{
    Roms roms;
    roms.add_installed();

    bool failed = false;

    for (const std::shared_ptr<Rom> &rom : roms.roms) {
        if (rom->system_is_image) {
            std::string mount_point(IMAGES_MOUNT_POINT);
            mount_point += "/";
            mount_point += rom->id;

            if (!mount_image(rom->full_system_path(), mount_point, 0771)) {
                LOGW("Failed to mount image for %s", rom->id.c_str());
                failed = true;
            }
        }
    }

    return !failed;
}

static bool disable_fsck(const char *fsck_binary)
{
    struct stat sb;
    if (stat(fsck_binary, &sb) < 0) {
        LOGE("%s: Failed to stat: %s", fsck_binary, strerror(errno));
        return errno == ENOENT;
    }

    std::string filename = util::base_name(fsck_binary);
    std::string path("/fscks/");
    path += filename;

    file_ptr fp(fopen(path.c_str(), "wb"), fclose);
    if (!fp) {
        LOGE("%s: Failed to open for writing: %s",
             path.c_str(), strerror(errno));
        return false;
    }

    fprintf(
        fp.get(),
        "#!/system/bin/sh\n"
        "echo %s was disabled by mbtool because performing an online fsck while the\n"
        "echo vfat partition is mounted is not possible. This is done to ensure that vold\n"
        "echo does not fail the filesystem checks and make the external SD invisible to the OS.\n"
        "echo If the filesystem checks must be completed, it will need to be done from a\n"
        "echo computer or from recovery.\n"
        "exit 0",
        filename.c_str()
    );
    fchmod(fileno(fp.get()), 0755);

    if (mount(path.c_str(), fsck_binary, "", MS_BIND | MS_RDONLY, "") < 0) {
        LOGE("Failed to bind mount %s to %s: %s",
             path.c_str(), fsck_binary, strerror(errno));
        return false;
    }

    return true;
}

static bool disable_fscks()
{
    // Online fsck is not possible so we'll have to prevent Vold from trying
    // to run fsck_msdos and failing.
    if (mkdir("/fscks", 0755) < 0 || chmod("/fscks", 0755) < 0) {
        return false;
    }

    if (mount("tmpfs", "/fscks", "tmpfs", MS_NOSUID, "mode=0755") < 0) {
        LOGE("%s: Failed to mount tmpfs: %s", "/fscks", strerror(errno));
        return false;
    }

    bool ret = disable_fsck("/system/bin/fsck_msdos");

    if (mount("", "/fscks", "", MS_REMOUNT | MS_RDONLY, "") < 0) {
        LOGW("%s: Failed to remount read-only: %s", "/fscks", strerror(errno));
    }

    return ret;
}

bool mount_fstab(const std::string &fstab_path, bool overwrite_fstab)
{
    std::vector<util::fstab_rec> fstab;
    std::vector<util::fstab_rec> generic_system(generic_fstab_system_entries());
    std::vector<util::fstab_rec> generic_cache(generic_fstab_cache_entries());
    std::vector<util::fstab_rec> generic_data(generic_fstab_data_entries());
    std::vector<util::fstab_rec *> recs_gen;
    std::vector<util::fstab_rec *> recs_system;
    std::vector<util::fstab_rec *> recs_cache;
    std::vector<util::fstab_rec *> recs_data;
    std::vector<util::fstab_rec *> recs_extsd;
    std::string path_fstab_gen;
    std::string path_completed;
    std::string base_name;
    std::string dir_name;
    struct stat st;
    std::shared_ptr<Rom> rom;

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
        LOGD("fstab: %s", rec.orig_line.c_str());
        if (rec.mount_point == "/system") {
            LOGD("-> /system entry");
            recs_system.push_back(&rec);
        } else if (rec.mount_point == "/cache") {
            LOGD("-> /cache entry");
            recs_cache.push_back(&rec);
        } else if (rec.mount_point == "/data") {
            LOGD("-> /data entry");
            recs_data.push_back(&rec);
        } else if (rec.vold_args.find("voldmanaged=sdcard1") != std::string::npos
                || rec.vold_args.find("voldmanaged=extSdCard") != std::string::npos
                || rec.vold_args.find("voldmanaged=external_SD") != std::string::npos) {
            LOGD("-> External SD entry");
            recs_extsd.push_back(&rec);
            recs_gen.push_back(&rec);
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

    // Some ROMs mount the partitions in one of the init.*.rc files or some
    // shell script. If that's the case, we just have to guess for working
    // fstab entries.
    if (recs_system.empty()) {
        LOGW("No /system fstab entries found. Adding generic entries");
        for (util::fstab_rec &rec : generic_system) {
            recs_system.push_back(&rec);
        }
    }
    if (recs_cache.empty()) {
        LOGW("No /cache fstab entries found. Adding generic entries");
        for (util::fstab_rec &rec : generic_cache) {
            recs_cache.push_back(&rec);
        }
    }
    if (recs_data.empty()) {
        LOGW("No /data fstab entries found. Adding generic entries");
        for (util::fstab_rec &rec : generic_data) {
            recs_data.push_back(&rec);
        }
    }

    // /system and /data are always in the fstab. The patcher should create
    // an entry for /cache for the ROMs that mount it manually in one of the
    // init scripts
    if (recs_system.empty() || recs_cache.empty() || recs_data.empty()) {
        LOGE("fstab does not contain all of /system, /cache, and /data!");
        return false;
    }

    if (rom->system_source == Rom::Source::CACHE) {
        for (util::fstab_rec *rec : recs_cache) {
            rec->flags &= ~MS_NOSUID;
        }
    } else if (rom->system_source == Rom::Source::DATA) {
        for (util::fstab_rec *rec : recs_data) {
            rec->flags &= ~MS_NOSUID;
        }
    }
    if (rom->cache_source == Rom::Source::SYSTEM) {
        for (util::fstab_rec *rec : recs_system) {
            rec->flags &= ~MS_RDONLY;
        }
    }

    if (!mount_fstab_entries(recs_system, recs_cache, recs_data)) {
        return false;
    }
    if (!mount_extsd_fstab_entries(recs_extsd)) {
        return false;
    }

    if (!mount_rom(rom)) {
        return false;
    }

    mount_all_system_images();
    disable_fscks();

    if (!apply_global_app_sharing(rom)) {
        return false;
    }

    // Set property for the Android app to use
    if (!util::set_property("ro.multiboot.romid", rom->id)) {
        LOGE("Failed to set 'ro.multiboot.romid' to '%s'", rom->id.c_str());
        file_ptr fp(fopen("/default.prop", "a"), fclose);
        if (fp) {
            fprintf(fp.get(), "\nro.multiboot.romid=%s\n", rom->id.c_str());
        }
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
