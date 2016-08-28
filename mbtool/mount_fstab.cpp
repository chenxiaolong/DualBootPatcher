/*
 * Copyright (C) 2014-2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mblog/logging.h"
#include "mbutil/autoclose/file.h"
#include "mbutil/command.h"
#include "mbutil/copy.h"
#include "mbutil/directory.h"
#include "mbutil/file.h"
#include "mbutil/finally.h"
#include "mbutil/fstab.h"
#include "mbutil/mount.h"
#include "mbutil/path.h"
#include "mbutil/properties.h"
#include "mbutil/selinux.h"
#include "mbutil/string.h"

#include "multiboot.h"
#include "reboot.h"
#include "roms.h"
#include "sepolpatch.h"
#include "signature.h"
#include "initwrapper/devices.h"

#define SYSTEM_MOUNT_POINT          "/raw/system"
#define CACHE_MOUNT_POINT           "/raw/cache"
#define DATA_MOUNT_POINT            "/raw/data"
#define EXTSD_MOUNT_POINT           "/raw/extsd"
#define IMAGES_MOUNT_POINT          "/raw/images"

#define EXT4_TEMP_IMAGE             "/temp.ext4"

#define WRAPPED_BINARIES_DIR        "/wrapped"


namespace mb
{

/*!
 * \brief Try mounting each entry in a list of fstab entry until one works.
 *
 * \param recs List of fstab entries for the given mount point
 * \param mount_point Target mount point
 *
 * \return Whether some fstab entry was successfully mounted at the mount point
 */
static bool create_dir_and_mount(const std::vector<util::fstab_rec> &recs,
                                 const char *mount_point, mode_t perms)
{
    if (recs.empty()) {
        return false;
    }

    LOGD("%s: Has %zu fstab entries", mount_point, recs.size());

    if (mkdir(mount_point, perms) < 0) {
        if (errno != EEXIST) {
            LOGE("%s: Failed to create directory: %s",
                 mount_point, strerror(errno));
            return false;
        } else if (chmod(mount_point, perms) < 0) {
            LOGE("%s: Failed to chmod: %s",
                 mount_point, strerror(errno));
            return false;
        }
    }

    // Try mounting each until we find one that works
    for (const util::fstab_rec &rec : recs) {
        LOGD("Attempting to mount(%s, %s, %s, %lu, %s)",
             rec.blk_device.c_str(), mount_point, rec.fs_type.c_str(),
             rec.flags, rec.fs_options.c_str());

        // Wait for block device if requested
        if (rec.fs_mgr_flags & MF_WAIT) {
            LOGD("%s: Waiting up to 20 seconds for block device",
                 rec.blk_device.c_str());
            util::wait_for_path(rec.blk_device.c_str(), 20 * 1000);
        }

        // Try mounting
        int ret = mount(rec.blk_device.c_str(),
                        mount_point,
                        rec.fs_type.c_str(),
                        rec.flags,
                        rec.fs_options.c_str());
        if (ret < 0) {
            LOGE("Failed to mount %s (%s) at %s: %s",
                 rec.blk_device.c_str(), rec.fs_type.c_str(),
                 mount_point, strerror(errno));
            continue;
        } else {
            LOGE("Successfully mounted %s (%s) at %s",
                 rec.blk_device.c_str(), rec.fs_type.c_str(),
                 mount_point);
            return true;
        }
    }

    LOGD("%s: Failed to mount all %zu fstab entries",
         mount_point, recs.size());

    return false;
}

/*!
 * \brief Get list of generic /system fstab entries for ROMs that mount the
 *        partition manually
 */
static std::vector<util::fstab_rec> generic_fstab_system_entries()
{
    std::string encoded;
    util::file_get_property(DEFAULT_PROP_PATH, PROP_BLOCK_DEV_SYSTEM_PATHS,
                            &encoded, "");
    std::vector<std::string> block_devs = decode_list(encoded);

    std::vector<util::fstab_rec> result;

    for (const std::string &block_dev : block_devs) {
        // ext4 entry
        result.emplace_back();
        result.back().blk_device = block_dev;
        result.back().mount_point = "/system";
        result.back().fs_type = "ext4";
        result.back().flags = MS_RDONLY;
        result.back().fs_options = "noauto_da_alloc,nodiscard,data=ordered,errors=panic";
        result.back().fs_mgr_flags = 0;
        result.back().vold_args = "check";
        // f2fs entry
        result.emplace_back();
        result.back().blk_device = block_dev;
        result.back().mount_point = "/system";
        result.back().fs_type = "f2fs";
        result.back().flags = MS_RDONLY;
        result.back().fs_options = "background_gc=off,nodiscard";
        result.back().fs_mgr_flags = 0;
        result.back().vold_args = "check";
    }

    return result;
}

/*!
 * \brief Get list of generic /cache fstab entries for ROMs that mount the
 *        partition manually
 */
static std::vector<util::fstab_rec> generic_fstab_cache_entries()
{
    std::string encoded;
    util::file_get_property(DEFAULT_PROP_PATH, PROP_BLOCK_DEV_CACHE_PATHS,
                            &encoded, "");
    std::vector<std::string> block_devs = decode_list(encoded);

    std::vector<util::fstab_rec> result;

    for (const std::string &block_dev : block_devs) {
        // ext4 entry
        result.emplace_back();
        result.back().blk_device = block_dev;
        result.back().mount_point = "/cache";
        result.back().fs_type = "ext4";
        result.back().flags = MS_NOSUID | MS_NODEV;
        result.back().fs_options = "noauto_da_alloc,discard,data=ordered,errors=panic";
        result.back().fs_mgr_flags = 0;
        result.back().vold_args = "check";
        // f2fs entry
        result.emplace_back();
        result.back().blk_device = block_dev;
        result.back().mount_point = "/cache";
        result.back().fs_type = "f2fs";
        result.back().flags = MS_NOSUID | MS_NODEV;
        result.back().fs_options = "background_gc=on,discard";
        result.back().fs_mgr_flags = 0;
        result.back().vold_args = "check";
    }

    return result;
}

/*!
 * \brief Get list of generic /data fstab entries for ROMs that mount the
 *        partition manually
 */
static std::vector<util::fstab_rec> generic_fstab_data_entries()
{
    std::string encoded;
    util::file_get_property(DEFAULT_PROP_PATH, PROP_BLOCK_DEV_DATA_PATHS,
                            &encoded, "");
    std::vector<std::string> block_devs = decode_list(encoded);

    std::vector<util::fstab_rec> result;

    for (const std::string &block_dev : block_devs) {
        // ext4 entry
        result.emplace_back();
        result.back().blk_device = block_dev;
        result.back().mount_point = "/data";
        result.back().fs_type = "ext4";
        result.back().flags = MS_NOSUID | MS_NODEV;
        result.back().fs_options = "noauto_da_alloc,discard,data=ordered,errors=panic";
        result.back().fs_mgr_flags = 0;
        result.back().vold_args = "check";
        // f2fs entry
        result.emplace_back();
        result.back().blk_device = block_dev;
        result.back().mount_point = "/data";
        result.back().fs_type = "f2fs";
        result.back().flags = MS_NOSUID | MS_NODEV;
        result.back().fs_options = "background_gc=on,discard";
        result.back().fs_mgr_flags = 0;
        result.back().vold_args = "check";
    }

    return result;
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

static void dump(const char *line, bool error, void *userdata)
{
    (void) error;
    (void) userdata;

    size_t size = strlen(line);

    std::string copy;
    if (size > 0 && line[size - 1] == '\n') {
        copy.assign(line, line + size - 1);
    } else {
        copy.assign(line, line + size);
    }

    LOGD("Command output: %s", copy.c_str());
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

static bool mount_exfat_fuse(const char *source, const char *target)
{
    uid_t uid = get_media_rw_uid();

    // Check signatures
    SigVerifyResult result;
    result = verify_signature("/sbin/fsck.exfat", "/sbin/fsck.exfat.sig");
    if (result != SigVerifyResult::VALID) {
        LOGE("Invalid fsck.exfat signature");
        return false;
    }
    result = verify_signature("/sbin/mount.exfat", "/sbin/mount.exfat.sig");
    if (result != SigVerifyResult::VALID) {
        LOGE("Invalid mount.exfat signature");
        return false;
    }

    char mount_args[100];

    snprintf(mount_args, sizeof(mount_args),
             "noatime,nodev,nosuid,dirsync,uid=%d,gid=%d,fmask=%o,dmask=%o,%s,%s",
             uid, uid, 0007, 0007, "noexec", "rw");

    const char *fsck_argv[] = {
        "/sbin/fsck.exfat",
        source,
        nullptr
    };
    const char *mount_argv[] = {
        "/sbin/mount.exfat",
        "-o", mount_args,
        source, target,
        nullptr
    };

    // Run filesystem checks
    util::run_command(fsck_argv[0], fsck_argv, nullptr, nullptr, &dump,
                      nullptr);

    // Mount exfat, matching vold options as much as possible
    int ret = util::run_command(mount_argv[0], mount_argv, nullptr, nullptr,
                                &dump, nullptr);

    if (ret >= 0) {
        LOGD("mount.exfat returned: %d", WEXITSTATUS(ret));
    }

    if (ret < 0) {
        LOGE("Failed to launch /sbin/mount.exfat: %s", strerror(errno));
        return false;
    } else if (WEXITSTATUS(ret) != 0) {
        LOGE("Failed to mount %s (%s) at %s",
             source, "fuse-exfat", target);
        return false;
    } else {
        LOGE("Successfully mounted %s (%s) at %s",
             source, "fuse-exfat", target);
        return true;
    }
}

static bool mount_exfat_kernel(const char *source, const char *target)
{
    uid_t uid = get_media_rw_uid();
    std::string args = util::format(
            "uid=%d,gid=%d,fmask=%o,dmask=%o,namecase=0",
            uid, uid, 0007, 0007);
    // For Motorola: utf8
    int flags = MS_NODEV
            | MS_NOSUID
            | MS_DIRSYNC
            | MS_NOEXEC;
    // For Motorola: MS_RELATIME

    int ret = mount(source, target, "exfat", flags, args.c_str());
    if (ret < 0) {
        LOGE("Failed to mount %s (%s) at %s: %s",
             source, "exfat", target, strerror(errno));
        return false;
    } else {
        LOGE("Successfully mounted %s (%s) at %s",
             source, "exfat", target);
        return true;
    }
}

static bool mount_vfat(const char *source, const char *target)
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

    int ret = mount(source, target, "vfat", flags, args.c_str());
    if (ret < 0) {
        LOGE("Failed to mount %s (%s) at %s: %s",
             source, "vfat", target, strerror(errno));
        return false;
    } else {
        LOGE("Successfully mounted %s (%s) at %s",
             source, "vfat", target);
        return true;
    }
}

static bool mount_ext4(const char *source, const char *target)
{
    int ret = mount(source, target, "ext4", 0, "");
    if (ret < 0) {
        LOGE("Failed to mount %s (%s) at %s: %s",
             source, "ext4", target, strerror(errno));
        return false;
    } else {
        LOGE("Successfully mounted %s (%s) at %s",
             source, "ext4", target);
        return true;
    }
}

static bool try_extsd_mount(const char *block_dev, const char *mount_point)
{
    // Vold ignores the fstab fstype field and uses blkid to determine the
    // filesystem. We don't link in blkid, so we'll use a trial and error
    // approach.

    bool use_fuse_exfat =
            util::file_find_one_of("/init.orig", { "EXFAT   ", "exfat" });
    std::string value;
    if (util::file_get_property(
            DEFAULT_PROP_PATH, PROP_USE_FUSE_EXFAT, &value, "false")) {
        LOGD("%s contains fuse-exfat override: %s",
             DEFAULT_PROP_PATH, value.c_str());
        if (value == "true") {
            use_fuse_exfat = true;
        } else if (value == "false") {
            use_fuse_exfat = false;
        } else {
            LOGW("Invalid '" PROP_USE_FUSE_EXFAT "' value: '%s'",
                 value.c_str());
        }
    } else {
        LOGW("%s: Failed to read properties: %s",
             DEFAULT_PROP_PATH, strerror(errno));
    }

    LOGD("Using fuse-exfat: %d", use_fuse_exfat);

    if (use_fuse_exfat && mount_exfat_fuse(block_dev, mount_point)) {
        return true;
    }
    if (mount_exfat_kernel(block_dev, mount_point)) {
        return true;
    }
    if (mount_vfat(block_dev, mount_point)) {
        return true;
    }
    if (mount_ext4(block_dev, mount_point)) {
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
static bool mount_extsd_fstab_entries(const std::vector<util::fstab_rec> &extsd_recs,
                                      const char *mount_point, mode_t perms)
{
    if (extsd_recs.empty()) {
        LOGD("No external SD fstab entries to mount");
        return true;
    }

    LOGD("%zu fstab entries for the external SD", extsd_recs.size());

    if (!util::mkdir_recursive(mount_point, perms)) {
        LOGE("%s: Failed to create directory: %s",
             mount_point, strerror(errno));
        return false;
    }

    // If an actual SD card was found and the mount operation failed, then set
    // to false. This way, the function won't fail if no SD card is inserted.
    bool failed = false;

    auto const *devices_map = get_devices_map();

    for (const util::fstab_rec &rec : extsd_recs) {
        std::vector<std::string> patterns =
                split_patterns(rec.blk_device.c_str());
        for (const std::string &pattern : patterns) {
            bool matched = false;

            for (auto const &pair : *devices_map) {
                if (path_matches(pair.first.c_str(), pattern.c_str())) {
                    matched = true;
                    const std::string &block_dev = pair.second;

                    if (try_extsd_mount(block_dev.c_str(), mount_point)) {
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

static bool mount_image(const char *image, const char *mount_point,
                        mode_t perms)
{
    if (!util::mkdir_recursive(mount_point, perms)) {
        LOGE("%s: Failed to create directory: %s",
             mount_point, strerror(errno));
        return false;
    }

    // Our image files are always ext4 images
    if (!util::mount(image, mount_point, "ext4", 0, "")) {
        LOGE("%s: Failed to mount image: %s", image, strerror(errno));
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
            std::string system_path(rom->full_system_path());

            if (!mount_image(system_path.c_str(), mount_point.c_str(), 0771)) {
                LOGW("Failed to mount image for %s", rom->id.c_str());
                failed = true;
            }
        }
    }

    return !failed;
}

static bool create_ext4_temp_fs(const char *mount_point)
{
    struct stat sb;
    if (stat(EXT4_TEMP_IMAGE, &sb) < 0) {
        if (errno == ENOENT) {
            const char *argv[] = {
                "/system/bin/make_ext4fs",
                "-l", "20M",
                EXT4_TEMP_IMAGE,
                nullptr
            };

            int ret = util::run_command(argv[0], argv, nullptr, nullptr, &dump,
                                        nullptr);
            if (ret < 0) {
                LOGE("Failed to run make_ext4fs");
                return false;
            } else if (WEXITSTATUS(ret) != 0) {
                LOGE("make_ext4fs returned non-zero exit status: %d",
                     WEXITSTATUS(ret));
                return false;
            }
        } else {
            LOGE("%s: Failed to stat: %s", EXT4_TEMP_IMAGE, strerror(errno));
        }
    }

    return util::mount(EXT4_TEMP_IMAGE, mount_point, "ext4", MS_NOSUID, "");
}

static bool create_tmpfs_temp_fs(const char *mount_point)
{
    return util::mount("tmpfs", mount_point, "tmpfs", MS_NOSUID, "mode=0755");
}

static bool create_temporary_fs(const char *mount_point)
{
    if (!create_ext4_temp_fs(mount_point)) {
        LOGW("Failed to create ext4-backed temporary filesystem");
        if (!create_tmpfs_temp_fs(mount_point)) {
            LOGW("Failed to create tmpfs-backed temporary filesystem");
            return false;
        }
    }
    return true;
}

static bool disable_fsck(const char *fsck_binary)
{
    struct stat sb;
    if (stat(fsck_binary, &sb) < 0) {
        LOGE("%s: Failed to stat: %s", fsck_binary, strerror(errno));
        return errno == ENOENT;
    }

    std::string filename = util::base_name(fsck_binary);
    std::string path(WRAPPED_BINARIES_DIR);
    path += "/";
    path += filename;

    autoclose::file fp(autoclose::fopen(path.c_str(), "wbe"));
    if (!fp) {
        LOGE("%s: Failed to open for writing: %s",
             path.c_str(), strerror(errno));
        return false;
    }

    fprintf(
        fp.get(),
        "#!/system/bin/sh\n"
        "echo %s was disabled by mbtool because performing an online fsck while the\n"
        "echo partition is mounted is not possible. This is done to ensure that vold\n"
        "echo does not fail the filesystem checks and make the external SD invisible to the OS.\n"
        "echo If the filesystem checks must be completed, it will need to be done from a\n"
        "echo computer or from recovery.\n"
        "exit 0",
        filename.c_str()
    );

    // Copy permissions
    fchown(fileno(fp.get()), sb.st_uid, sb.st_gid);
    fchmod(fileno(fp.get()), sb.st_mode);

    // Copy SELinux label
    std::string context;
    if (util::selinux_get_context(fsck_binary, &context)) {
        LOGD("%s: SELinux label is: %s", fsck_binary, context.c_str());
        if (!util::selinux_fset_context(fileno(fp.get()), context)) {
            LOGW("%s: Failed to set SELinux label: %s",
                 path.c_str(), strerror(errno));
        }
    } else {
        LOGW("%s: Failed to get SELinux label: %s",
             fsck_binary, strerror(errno));
    }

    if (mount(path.c_str(), fsck_binary, "", MS_BIND | MS_RDONLY, "") < 0) {
        LOGE("Failed to bind mount %s to %s: %s",
             path.c_str(), fsck_binary, strerror(errno));
        return false;
    }

    return true;
}

static bool copy_mount_exfat()
{
    const char *system_mount_exfat = "/system/bin/mount.exfat";
    const char *our_mount_exfat = "/sbin/mount.exfat";
    const char *target = WRAPPED_BINARIES_DIR "/mount.exfat";

    struct stat sb;
    if (stat(system_mount_exfat, &sb) < 0) {
        LOGE("%s: Failed to stat: %s", system_mount_exfat, strerror(errno));
        return errno == ENOENT;
    }

    if (!util::copy_file(our_mount_exfat, target, 0)) {
        return false;
    }

    // Copy permissions
    chown(target, sb.st_uid, sb.st_gid);
    chmod(target, sb.st_mode);

    // Copy SELinux label
    std::string context;
    if (util::selinux_get_context(system_mount_exfat, &context)) {
        LOGD("%s: SELinux label is: %s", system_mount_exfat, context.c_str());
        if (!util::selinux_set_context(target, context)) {
            LOGW("%s: Failed to set SELinux label: %s",
                 target, strerror(errno));
        }
    } else {
        LOGW("%s: Failed to get SELinux label: %s",
             system_mount_exfat, strerror(errno));
    }

    if (mount(target, system_mount_exfat, "", MS_BIND | MS_RDONLY, "") < 0) {
        LOGE("Failed to bind mount %s to %s: %s",
             target, system_mount_exfat, strerror(errno));
        return false;
    }

    return true;
}

static bool wrap_binaries()
{
    if (mkdir(WRAPPED_BINARIES_DIR, 0755) < 0 && errno != EEXIST) {
        return false;
    }
    if (!create_temporary_fs(WRAPPED_BINARIES_DIR)) {
        return false;
    }

    bool ret;

    // Online fsck is not possible so we'll have to prevent Vold from trying
    // to run fsck_msdos and failing.
    if (!disable_fsck("/system/bin/fsck_msdos")) {
        ret = false;
    }
    if (!disable_fsck("/system/bin/fsck.exfat")) {
        ret = false;
    }

    // Ensure our copy of mount.exfat is used
    if (!copy_mount_exfat()) {
        ret = false;
    }

    // Remount read-only to avoid further modifications to the temporary dir
    if (mount("", WRAPPED_BINARIES_DIR, "", MS_REMOUNT | MS_RDONLY, "") < 0) {
        LOGW("%s: Failed to remount read-only: %s", WRAPPED_BINARIES_DIR,
             strerror(errno));
    }

    return ret;
}

struct FstabRecs
{
    // Entries to go in newly generated fstab
    std::vector<util::fstab_rec> gen;
    // /system entries
    std::vector<util::fstab_rec> system;
    // /cache entries
    std::vector<util::fstab_rec> cache;
    // /data entries
    std::vector<util::fstab_rec> data;
    // External SD entries
    std::vector<util::fstab_rec> extsd;
};

bool process_fstab(const char *path, const std::shared_ptr<Rom> &rom, int flags,
                   FstabRecs *recs)
{
    std::vector<util::fstab_rec> fstab;

    recs->gen.clear();
    recs->system.clear();
    recs->cache.clear();
    recs->data.clear();
    recs->extsd.clear();

    // Read original fstab file
    fstab = util::read_fstab(path);
    if (fstab.empty()) {
        LOGE("%s: Failed to read fstab", path);
        return false;
    }

    for (auto it = fstab.begin(); it != fstab.end();) {
        LOGD("fstab: %s", it->orig_line.c_str());

        if (util::path_compare(it->mount_point, "/system") == 0
                && (flags & MOUNT_FLAG_MOUNT_SYSTEM)) {
            LOGD("-> /system entry");
            recs->system.push_back(std::move(*it));
            it = fstab.erase(it);
        } else if (util::path_compare(it->mount_point, "/cache") == 0
                && (flags & MOUNT_FLAG_MOUNT_CACHE)) {
            LOGD("-> /cache entry");
            recs->cache.push_back(std::move(*it));
            it = fstab.erase(it);
        } else if (util::path_compare(it->mount_point, "/data") == 0
                && (flags & MOUNT_FLAG_MOUNT_DATA)) {
            LOGD("-> /data entry");
            recs->data.push_back(std::move(*it));
            it = fstab.erase(it);
        } else if ((it->vold_args.find("voldmanaged=sdcard0") != std::string::npos
                || it->vold_args.find("voldmanaged=sdcard1") != std::string::npos
                || it->vold_args.find("voldmanaged=extSdCard") != std::string::npos
                || it->vold_args.find("voldmanaged=external_SD") != std::string::npos
                || it->vold_args.find("voldmanaged=MicroSD") != std::string::npos)
                && (flags & MOUNT_FLAG_MOUNT_EXTERNAL_SD)) {
            LOGD("-> External SD entry");
            // Has to be mounted by us
            recs->extsd.push_back(*it);
            // and also has to be processed by vold
            recs->gen.push_back(std::move(*it));
            it = fstab.erase(it);
        } else {
            // Let vold mount this
            recs->gen.push_back(std::move(*it));
            it = fstab.erase(it);
        }
    }

    // Some ROMs mount the partitions in one of the init.*.rc files or some
    // shell script. If that's the case, we just have to guess for working
    // fstab entries.
    if (!(flags & MOUNT_FLAG_NO_GENERIC_ENTRIES)) {
        if (recs->system.empty() && (flags & MOUNT_FLAG_MOUNT_SYSTEM)) {
            LOGW("No /system fstab entries found. Adding generic entries");
            auto entries = generic_fstab_system_entries();
            for (util::fstab_rec &rec : entries) {
                recs->system.push_back(std::move(rec));
            }
        }
        if (recs->cache.empty() && (flags & MOUNT_FLAG_MOUNT_CACHE)) {
            LOGW("No /cache fstab entries found. Adding generic entries");
            auto entries = generic_fstab_cache_entries();
            for (util::fstab_rec &rec : entries) {
                recs->cache.push_back(std::move(rec));
            }
        }
        if (recs->data.empty() && (flags & MOUNT_FLAG_MOUNT_DATA)) {
            LOGW("No /data fstab entries found. Adding generic entries");
            auto entries = generic_fstab_data_entries();
            for (util::fstab_rec &rec : entries) {
                recs->data.push_back(std::move(rec));
            }
        }
    }

    // Remove nosuid flag on the partition that the system directory resides on
    if (rom && !rom->system_is_image) {
        if (rom->system_source == Rom::Source::CACHE) {
            for (util::fstab_rec &rec : recs->cache) {
                rec.flags &= ~MS_NOSUID;
            }
        } else if (rom->system_source == Rom::Source::DATA) {
            for (util::fstab_rec &rec : recs->data) {
                rec.flags &= ~MS_NOSUID;
            }
        }
    }

    // TODO: util::bind_mount() chmod's the source directory. Once that is
    // removed, we can use read-only system partitions again
    if (rom /* && rom->cache_source == Rom::Source::SYSTEM */) {
        for (util::fstab_rec &rec : recs->system) {
            rec.flags &= ~MS_RDONLY;
        }
    }

    return true;
}

/*!
 * \brief Mount system, cache, and data entries from fstab
 *
 * \param path Path to fstab file
 * \param flags Mount flags
 *
 * \return Whether all of the
 */
bool mount_fstab(const char *path, const std::shared_ptr<Rom> &rom, int flags)
{
    std::vector<std::string> successful;
    FstabRecs recs;

    if (!process_fstab(path, rom, flags, &recs)) {
        return false;
    }

    // Partitions are mounted in /raw
    if (mkdir("/raw", 0755) < 0 && errno != EEXIST) {
        LOGE("Failed to create /raw: %s", strerror(errno));
        return false;
    }

    bool ret = true;

    // Mount system
    if (ret && !recs.system.empty()) {
        if (create_dir_and_mount(recs.system, SYSTEM_MOUNT_POINT, 0755)) {
            successful.push_back(SYSTEM_MOUNT_POINT);
        } else {
            LOGE("Failed to mount " SYSTEM_MOUNT_POINT);
            ret = false;
        }
    }

    // Mount cache
    if (ret && !recs.cache.empty()) {
        if (create_dir_and_mount(recs.cache, CACHE_MOUNT_POINT, 0755)) {
            successful.push_back(CACHE_MOUNT_POINT);
        } else {
            LOGE("Failed to mount " CACHE_MOUNT_POINT);
            ret = false;
        }
    }

    // Mount data
    if (ret && !recs.data.empty()) {
        if (create_dir_and_mount(recs.data, DATA_MOUNT_POINT, 0755)) {
            successful.push_back(DATA_MOUNT_POINT);
        } else {
            LOGE("Failed to mount " DATA_MOUNT_POINT);
            ret = false;
        }
    }

    // Mount external SD
    if (ret && !recs.extsd.empty()) {
        if (mount_extsd_fstab_entries(recs.extsd, EXTSD_MOUNT_POINT, 0755)) {
            // Only add to list if an SD card exists and is mounted
            if (util::is_mounted(EXTSD_MOUNT_POINT)) {
                successful.push_back(EXTSD_MOUNT_POINT);
            }
        } else {
            LOGE("Failed to mount external SD");
            ret = false;
        }
    }

    if (ret) {
        LOGI("Successfully mounted partitions");
    } else if (flags & MOUNT_FLAG_UNMOUNT_ON_FAILURE) {
        for (const std::string &mount_point : successful) {
            util::umount(mount_point.c_str());
        }
    }

    // Rewrite fstab file
    if (ret && (flags & MOUNT_FLAG_REWRITE_FSTAB)) {
        int fd = open(path, O_RDWR | O_TRUNC);
        if (fd < 0) {
            LOGE("%s: Failed to open file: %s", path, strerror(errno));
            return false;
        }

        for (const util::fstab_rec &rec : recs.gen) {
            dprintf(fd, "%s\n", rec.orig_line.c_str());
        }

        close(fd);
    }

    return ret;
}

bool mount_rom(const std::shared_ptr<Rom> &rom)
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
        if (!mount_image(target_system.c_str(), "/system", 0771)) {
            return false;
        }
    } else {
        if (!util::bind_mount(target_system, 0771, "/system", 0771)) {
            return false;
        }
    }
    if (rom->cache_is_image) {
        if (!mount_image(target_cache.c_str(), "/cache", 0771)) {
            return false;
        }
    } else {
        if (!util::bind_mount(target_cache, 0771, "/cache", 0771)) {
            return false;
        }
    }
    if (rom->data_is_image) {
        if (!mount_image(target_data.c_str(), "/data", 0771)) {
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

    mount_all_system_images();
    wrap_binaries();

    return true;
}

}
