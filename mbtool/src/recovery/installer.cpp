/*
 * Copyright (C) 2014-2018  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "recovery/installer.h"

// C++
#include <algorithm>
#include <chrono>

// C
#include <cstring>

// Linux/posix
#include <dirent.h>
#include <fcntl.h>
#include <sched.h>
#include <signal.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/wait.h>
#include <unistd.h>

// Linux
#include <linux/loop.h>

// libmbcommon
#include "mbcommon/finally.h"
#include "mbcommon/string.h"
#include "mbcommon/version.h"

// libmblog
#include "mblog/logging.h"

// libmbdevice
#include "mbdevice/json.h"

// libmbutil
#include "mbutil/archive.h"
#include "mbutil/chmod.h"
#include "mbutil/chown.h"
#include "mbutil/command.h"
#include "mbutil/copy.h"
#include "mbutil/delete.h"
#include "mbutil/directory.h"
#include "mbutil/file.h"
#include "mbutil/fstab.h"
#include "mbutil/hash.h"
#include "mbutil/loopdev.h"
#include "mbutil/mount.h"
#include "mbutil/path.h"
#include "mbutil/properties.h"
#include "mbutil/string.h"

// Local
#include "recovery/image.h"
#include "recovery/installer_util.h"
#include "util/multiboot.h"
#include "util/signature.h"
#include "util/romconfig.h"
#include "util/switcher.h"
#include "util/wipe.h"

#define LOG_TAG "mbtool/recovery/installer"

// Set to 1 to spawn a shell after installation
// NOTE: This should ONLY be used through adb. For example:
//
//     $ adb push mbtool_recovery /tmp/updater
//     $ adb shell /tmp/updater 3 1 /path/to/file_patched.zip
#define DEBUG_PRE_SHELL 0
#define DEBUG_POST_SHELL 0

// Use an update-binary file not from the zip file
#define DEBUG_USE_ALTERNATE_UPDATER 0
#define DEBUG_ALTERNATE_UPDATER_PATH "/tmp/updater.orig"

// Use a wrapper around the updater
#define DEBUG_USE_UPDATER_WRAPPER 0
#define DEBUG_UPDATER_WRAPPER_PATH "/tmp/strace_static"
//#define DEBUG_UPDATER_WRAPPER_ARGS "-f" // Comma-separated strings


#define HELPER_TOOL             "/update-binary-tool"


using namespace mb::device;

namespace mb
{

using ScopedDIR = std::unique_ptr<DIR, decltype(closedir) *>;
using ScopedFILE = std::unique_ptr<FILE, decltype(fclose) *>;

const std::string Installer::CANCELLED = "cancelled";


Installer::Installer(std::string zip_file, std::string chroot_dir,
                     std::string temp_dir, int interface, int output_fd,
                     InstallerFlags flags)
    : _zip_file(std::move(zip_file))
    , _chroot(std::move(chroot_dir))
    , _temp(std::move(temp_dir))
    , _interface(interface)
    , _output_fd(output_fd)
    , _flags(flags)
    , _ran(false)
{
    _passthrough = _output_fd >= 0;

    LOGD("Initialized installer for zip file: %s", _zip_file.c_str());
}

Installer::~Installer()
{
}


/*
 * Wrappers around functions that log failures
 */

static int log_mkdir(const char *pathname, mode_t mode)
{
    int ret = mkdir(pathname, mode);
    if (ret < 0) {
        LOGE("Failed to create %s: %s", pathname, strerror(errno));
    }
    return ret;
}

static int log_mount(const char *source, const char *target, const char *fstype,
                     long unsigned int mountflags, const void *data)
{
    int ret = mount(source, target, fstype, mountflags, data);
    if (ret < 0) {
        LOGE("Failed to mount %s (%s) at %s: %s",
             source, fstype, target, strerror(errno));
    }
    return ret;
}

static int log_umount(const char *target)
{
    int ret = umount(target);
    if (ret < 0) {
        LOGE("Failed to unmount %s: %s", target, strerror(errno));
    }
    return ret;
}

static int log_mknod(const char *pathname, mode_t mode, dev_t dev)
{
    int ret = mknod(pathname, mode, dev);
    if (ret < 0) {
        LOGE("Failed to create special file %s: %s", pathname, strerror(errno));
    }
    return ret;
}

static bool log_is_mounted(const std::string &mountpoint)
{
    auto ret = util::is_mounted(mountpoint);
    if (!ret) {
        LOGE("%s: %s", mountpoint.c_str(), ret.error().message().c_str());
        return false;
    }
    return true;
}

static bool log_unmount_all(const std::string &dir)
{
    auto ret = util::unmount_all(dir);
    if (!ret) {
        LOGE("Failed to unmount all mountpoints within %s: %s",
             dir.c_str(), ret.error().message().c_str());
        return false;
    }
    return true;
}

static bool log_delete_recursive(const std::string &path)
{
    if (auto r = util::delete_recursive(path); !r) {
        LOGE("Failed to recursively remove %s: %s",
             path.c_str(), r.error().message().c_str());
        return false;
    }
    return true;
}

static bool log_copy_dir(const std::string &source,
                         const std::string &target, util::CopyFlags flags)
{
    if (auto r = util::copy_dir(source, target, flags); !r) {
        LOGE("Failed to copy contents of %s/ to %s/: %s",
             source.c_str(), target.c_str(), r.error().message().c_str());
        return false;
    }
    return true;
}


/*
 * Helper functions
 */

void Installer::output_cb(std::string_view line, bool error)
{
    (void) error;

    command_output(line);
}

int Installer::run_command(const std::vector<std::string> &argv)
{
    using namespace std::placeholders;

    return util::run_command(
        argv[0],
        argv,
        {},
        {},
        _passthrough ? util::CmdLineCb{}
            : std::bind(&Installer::output_cb, this, _1, _2)
    );
}

int Installer::run_command_chroot(const std::string &dir,
                                  const std::vector<std::string> &argv)
{
    using namespace std::placeholders;

    return util::run_command(
        argv[0],
        argv,
        {},
        dir,
        _passthrough ? util::CmdLineCb{}
            : std::bind(&Installer::output_cb, this, _1, _2)
    );
}

std::string Installer::in_chroot(const std::string &path) const
{
    if (!path.empty() && path[0] == '/') {
        return std::string(_chroot).append(path);
    } else {
        return std::string(_chroot).append("/").append(path);
    }
}

bool Installer::create_chroot()
{
    // We'll just call the recovery's mount tools directly to avoid having to
    // parse TWRP and CWM's different fstab formats
    run_command({ "mount", "/system" });
    run_command({ "mount", "/cache" });
    run_command({ "mount", "/data" });
    run_command({ "mount", "-o", "ro", "/efs" });

    // Remount as writable (needed for in-app flashing)
    log_mount("", Roms::get_system_partition().c_str(), "", MS_REMOUNT, "");
    log_mount("", Roms::get_cache_partition().c_str(), "", MS_REMOUNT, "");
    log_mount("", Roms::get_data_partition().c_str(), "", MS_REMOUNT, "");

    // Make sure everything really is mounted
    if (!log_is_mounted("/system")
            || !log_is_mounted("/cache")
            || !log_is_mounted("/data")) {
        return false;
    }

    // Unmount everything previously mounted in the chroot
    if (!log_unmount_all(_chroot)) {
        return false;
    }

    // Remove the chroot if it exists
    if (!log_delete_recursive(_chroot)) {
        return false;
    }

    // Create chroot and mount tmpfs there
    if (log_mkdir(_chroot.c_str(), 0700) < 0
            || log_mount("tmpfs", _chroot.c_str(), "tmpfs", 0, "") < 0) {
        return false;
    }

    // Create remaining directories
    if (log_mkdir(in_chroot("/mb").c_str(), 0755) < 0
            || log_mkdir(in_chroot("/dev").c_str(), 0755) < 0
            || log_mkdir(in_chroot("/etc").c_str(), 0755) < 0
            || log_mkdir(in_chroot("/proc").c_str(), 0755) < 0
            || log_mkdir(in_chroot("/sbin").c_str(), 0755) < 0
            || log_mkdir(in_chroot("/sys").c_str(), 0755) < 0
            || log_mkdir(in_chroot("/tmp").c_str(), 0755) < 0
            || log_mkdir(in_chroot("/data").c_str(), 0755) < 0
            || log_mkdir(in_chroot("/cache").c_str(), 0755) < 0
            || log_mkdir(in_chroot("/system").c_str(), 0755) < 0
            || log_mkdir(in_chroot("/firmware").c_str(), 0755) < 0
            || log_mkdir(in_chroot("/efs").c_str(), 0755) < 0) {
        return false;
    }

    // Other mounts
    if (log_mount("none", in_chroot("/dev").c_str(), "tmpfs", 0, "") < 0
            || log_mkdir(in_chroot("/dev/pts").c_str(), 0755) < 0
            || log_mount("none", in_chroot("/dev/pts").c_str(), "devpts", 0, "") < 0
            || log_mount("none", in_chroot("/proc").c_str(), "proc", 0, "") < 0
            || log_mount("none", in_chroot("/sys").c_str(), "sysfs", 0, "") < 0
            || log_mount("none", in_chroot("/tmp").c_str(), "tmpfs", 0, "") < 0) {
        return false;
    }

    // Some recoveries don't have SELinux enabled
    if (log_mount("none", in_chroot("/sys/fs/selinux").c_str(), "selinuxfs", 0, "") < 0
            && errno != ENOENT) {
        LOGV("Ignoring /sys/fs/selinux mount");
        LOGE("Failed to mount %s (%s) at %s: %s",
             "none", "selinuxfs", in_chroot("/sys/fs/selinux").c_str(),
             strerror(errno));
        return false;
    }

    // Copy the contents of sbin since we need to mess with some of the binaries
    // there. Also, for whatever reason, bind mounting /sbin results in EINVAL
    // no matter if it's done from here or from busybox.
    if (!log_copy_dir("/sbin", in_chroot("/sbin"),
                      util::CopyFlag::CopyAttributes
                    | util::CopyFlag::CopyXattrs
                    | util::CopyFlag::ExcludeTopLevel)) {
        return false;
    }

    // Remove reboot binary
    remove(in_chroot("/sbin/reboot").c_str());

    // Don't create unnecessary special files in /dev to avoid install scripts
    // from overwriting partitions
    if (log_mknod(in_chroot("/dev/console").c_str(), S_IFCHR | 0644, makedev(5, 1)) < 0
            || log_mknod(in_chroot("/dev/null").c_str(), S_IFCHR | 0644, makedev(1, 3)) < 0
            || log_mknod(in_chroot("/dev/ptmx").c_str(), S_IFCHR | 0644, makedev(5, 2)) < 0
            || log_mknod(in_chroot("/dev/random").c_str(), S_IFCHR | 0644, makedev(1, 8)) < 0
            || log_mknod(in_chroot("/dev/tty").c_str(), S_IFCHR | 0644, makedev(5, 0)) < 0
            || log_mknod(in_chroot("/dev/urandom").c_str(), S_IFCHR | 0644, makedev(1, 9)) < 0
            || log_mknod(in_chroot("/dev/zero").c_str(), S_IFCHR | 0644, makedev(1, 5)) < 0
            || log_mknod(in_chroot("/dev/loop-control").c_str(), S_IFCHR | 0644, makedev(10, 237)) < 0
            || log_mknod(in_chroot("/dev/fuse").c_str(), S_IFCHR | 0644, makedev(10, 229))) {
        return false;
    }

    // Create a few loopback devices since some installers expect them to exist,
    // but don't create them. They are not necessary for mbtool to work.
    if (log_mkdir(in_chroot("/dev/block").c_str(), 0755) < 0
            || log_mknod(in_chroot("/dev/block/loop0").c_str(), S_IFBLK | 0644, makedev(7, 0)) < 0
            || log_mknod(in_chroot("/dev/block/loop1").c_str(), S_IFBLK | 0644, makedev(7, 1)) < 0
            || log_mknod(in_chroot("/dev/block/loop2").c_str(), S_IFBLK | 0644, makedev(7, 2)) < 0
            || log_mknod(in_chroot("/dev/block/loop3").c_str(), S_IFBLK | 0644, makedev(7, 3)) < 0
            || log_mknod(in_chroot("/dev/block/loop4").c_str(), S_IFBLK | 0644, makedev(7, 4)) < 0
            || log_mknod(in_chroot("/dev/block/loop5").c_str(), S_IFBLK | 0644, makedev(7, 5)) < 0
            || log_mknod(in_chroot("/dev/block/loop6").c_str(), S_IFBLK | 0644, makedev(7, 6)) < 0
            || log_mknod(in_chroot("/dev/block/loop7").c_str(), S_IFBLK | 0644, makedev(7, 7)) < 0) {
        return false;
    }

    // We need /dev/input/* and /dev/graphics/* for AROMA
    if (!log_copy_dir("/dev/input", in_chroot("/dev/input"),
                      util::CopyFlag::CopyAttributes
                    | util::CopyFlag::CopyXattrs
                    | util::CopyFlag::ExcludeTopLevel)) {
        return false;
    }
    if (!log_copy_dir("/dev/graphics", in_chroot("/dev/graphics"),
                      util::CopyFlag::CopyAttributes
                    | util::CopyFlag::CopyXattrs
                    | util::CopyFlag::ExcludeTopLevel)) {
        return false;
    }

    // Mount EFS partition so patched Odin images can properly set up multi-CSC
    if (!mount_efs()) {
        return false;
    }

    (void) util::create_empty_file(in_chroot("/.chroot"));

    return true;
}

bool Installer::destroy_chroot() const
{
    // Disassociate loop devices that the ROM installer may have assigned
    // (grr, SuperSU...)
    std::string dev_block_path(in_chroot("/dev/block"));
    ScopedDIR dp(opendir(dev_block_path.c_str()), closedir);
    if (dp) {
        std::string path;
        struct dirent *ent;
        while ((ent = readdir(dp.get()))) {
            path = dev_block_path;
            path += '/';
            path += ent->d_name;
            (void) util::loopdev_remove_device(path.c_str());
        }
        dp.reset();
    }

    log_umount(in_chroot("/system").c_str());
    log_umount(in_chroot("/cache").c_str());
    log_umount(in_chroot("/data").c_str());
    log_umount(in_chroot("/efs").c_str());

    log_umount(in_chroot("/dev/pts").c_str());
    log_umount(in_chroot("/dev").c_str());
    log_umount(in_chroot("/proc").c_str());
    log_umount(in_chroot("/sys/fs/selinux").c_str());
    log_umount(in_chroot("/sys").c_str());
    log_umount(in_chroot("/tmp").c_str());
    log_umount(in_chroot("/sbin").c_str());

    log_umount(_chroot.c_str());

    // Unmount everything previously mounted in the chroot
    if (auto ret = util::unmount_all(_chroot); !ret) {
        LOGE("Failed to unmount previous mount points in %s: %s",
             _chroot.c_str(), ret.error().message().c_str());
        return false;
    }

    (void) util::delete_recursive(_chroot);

    if (log_is_mounted("/efs")) {
        log_umount("/efs");
    }

    return true;
}

bool Installer::mount_efs() const
{
    std::string manufacturer =
            util::property_get_string("ro.product.manufacturer", {});
    std::string brand = util::property_get_string("ro.product.brand", {});

    if (strcasecmp(manufacturer.c_str(), "samsung") != 0
            && strcasecmp(brand.c_str(), "samsung") != 0) {
        LOGD("Not mounting EFS partition because this is not a Samsung device");
        return true;
    }

    LOGD("This is a Samsung device; mounting EFS partition in chroot");

    struct stat sb;

    // Mount /efs as read-only if it exists
    if (log_is_mounted("/efs")) {
        if (log_mount("/efs", in_chroot("/efs").c_str(), "",
                      MS_BIND | MS_RDONLY, "") < 0) {
            return false;
        }
        LOGD("Bind mounted /efs to chroot as read-only");
    } else if (stat("/etc/recovery.fstab", &sb) == 0) {
        LOGD("Found /etc/recovery.fstab");

        std::string efs_dev;

        if (stat("/twres", &sb) == 0 && S_ISDIR(sb.st_mode)) {
            LOGD("Looking for /efs entry in TWRP-format fstab");

            if (auto recs = util::read_twrp_fstab("/etc/recovery.fstab")) {
                for (auto const &rec : recs.value()) {
                    if (util::path_compare(rec.mount_point, "/efs") == 0
                            || util::path_compare(rec.mount_point, "/efs1") == 0) {
                        LOGD("Found /efs fstab entry");
                        for (const std::string &dev : rec.blk_devices) {
                            if (stat(dev.c_str(), &sb) == 0) {
                                efs_dev = dev.c_str();
                                break;
                            }
                        }
                        break;
                    }
                }
            } else {
                LOGW("/etc/recovery.fstab: Failed to read fstab: %s",
                     recs.error().message().c_str());
            }
        } else {
            LOGE("Looking for /efs entry in non-TWRP-format fstab");

            if (auto recs = util::read_fstab("/etc/recovery.fstab")) {
                for (auto const &rec : recs.value()) {
                    if (util::path_compare(rec.mount_point, "/efs") == 0) {
                        LOGD("Found /efs fstab entry");
                        efs_dev = rec.blk_device;
                        break;
                    }
                }
            } else {
                LOGW("/etc/recovery.fstab: Failed to read fstab: %s",
                     recs.error().message().c_str());
            }
        }

        // FIXME: Temporary hack until the EFS partition path is specified in
        // the device definition
        std::vector<std::string> efs_devs{
            efs_dev,
            "/dev/block/bootdevice/by-name/efs",
            "/dev/block/bootdevice/by-name/EFS",
        };

        for (auto const &dev : efs_devs) {
            if (!dev.empty() && stat(dev.c_str(), &sb) == 0
                    && log_mount(dev.c_str(), in_chroot("/efs").c_str(),
                                 "ext4", MS_RDONLY, "") == 0) {
                return true;
            }
        }

        return false;
    }

    return true;
}

/*!
 * \brief Extract needed multiboot files from the patched zip file
 */
bool Installer::extract_multiboot_files()
{
    std::vector<util::ExtractInfo> files{
        {
            "META-INF/com/google/android/update-binary.orig",
            _temp + "/updater"
        },
        {
            "META-INF/com/google/android/update-binary",
            _temp + "/mbtool"
        },
        {
            "META-INF/com/google/android/update-binary.sig",
            _temp + "/mbtool.sig"
        },
        {
            "multiboot/bb-wrapper.sh",
            _temp + "/bb-wrapper.sh"
        },
        {
            "multiboot/bb-wrapper.sh.sig",
            _temp + "/bb-wrapper.sh.sig"
        },
        {
            "multiboot/device.json",
            _temp + "/device.json"
        },
        {
            "multiboot/info.prop",
            _temp + "/info.prop"
        },
    };

    std::vector<std::string> binaries{
        "file-contexts-tool",
        "file-contexts-tool.sig",
        "fsck-wrapper",
        "fsck-wrapper.sig",
        "mbtool",
        "mbtool.sig",
        "mount.exfat",
        "mount.exfat.sig",
    };

    for (auto const &binary : binaries) {
        files.push_back({
            "multiboot/binaries/" + binary,
            _temp + "/binaries/" + binary
        });
    }

    if (!util::extract_files2(_zip_file, files)) {
        LOGE("Failed to extract all multiboot files");
        return false;
    }

    std::vector<std::string> sigcheck{
        _temp + "/mbtool",
        _temp + "/bb-wrapper.sh",
        _temp + "/binaries/file-contexts-tool",
        _temp + "/binaries/fsck-wrapper",
        _temp + "/binaries/mbtool",
        _temp + "/binaries/mount.exfat",
    };

    for (auto const &item : sigcheck) {
        SigVerifyResult result =
                verify_signature(item.c_str(), (item + ".sig").c_str());
        if (result != SigVerifyResult::Valid) {
            LOGE("%s: Signature verification failed", item.c_str());
            return false;
        }
    }

    return true;
}

/*!
 * \brief Replace busybox in the chroot with a wrapper that disables certain
 *        functions
 */
bool Installer::set_up_busybox_wrapper()
{
    std::string temp_busybox = _temp + "/bb-wrapper.sh";
    std::string sbin_busybox = in_chroot("/sbin/busybox");

    rename(sbin_busybox.c_str(), in_chroot("/sbin/busybox_orig").c_str());

    if (auto r = util::copy_file(temp_busybox, sbin_busybox,
                                 util::CopyFlag::CopyAttributes
                               | util::CopyFlag::CopyXattrs); !r) {
        LOGE("%s", r.error().message().c_str());
        return false;
    }

    if (chmod(sbin_busybox.c_str(), 0555) < 0) {
        LOGE("%s: Failed to chmod: %s", sbin_busybox.c_str(), strerror(errno));
        return false;
    }

    return true;
}

/*!
 * \brief Create temporary ext4 image
 *
 * \param path Image file path
 */
bool Installer::create_image(const std::string &path, uint64_t size)
{
    if (auto r = util::mkdir_parent(path, S_IRWXU); !r) {
        LOGE("%s: Failed to create parent directory: %s",
             path.c_str(), r.error().message().c_str());
        return false;
    }

    auto result = create_ext4_image(path, size);
    if (result == CreateImageResult::NotEnoughSpace) {
        auto avail = util::mount_get_avail_size(util::dir_name(path));

        display_msg(std::string{});
        display_msg("There is not enough space to create %s", path.c_str());
        display_msg("- Needed:    %" PRIu64 " bytes", size);
        display_msg("- Available: %" PRIu64 " bytes", avail ? avail.value() : 0);
    }

    return result == CreateImageResult::Succeeded;
}

/*!
 * \brief Copy a /system directory to an image file
 *
 * \param source Source directory
 * \param image Target image file
 * \param reverse If non-zero, then the image file is the source and the
 *                directory is the target
 */
bool Installer::system_image_copy(const std::string &source,
                                  const std::string &image, bool reverse)
{
    std::string temp_mnt(_temp);
    temp_mnt += "/.system.tmp";

    if (auto r = util::mkdir_recursive(source, 0755);
            !r && r.error() != std::errc::file_exists) {
        LOGE("Failed to create %s: %s", source.c_str(),
             r.error().message().c_str());
        return false;
    }

    if (mkdir(temp_mnt.c_str(), 0755) < 0 && errno != EEXIST) {
        LOGE("Failed to create %s: %s", temp_mnt.c_str(), strerror(errno));
        return false;
    }

    if (auto ret = util::mount(image, temp_mnt, "auto", 0, ""); !ret) {
        LOGE("Failed to mount %s: %s", source.c_str(),
             ret.error().message().c_str());
        return false;
    }

    auto unmount_tmp_dir = finally([&] {
        (void) util::umount(temp_mnt);
    });

    if (reverse) {
        if (!copy_system(temp_mnt, source)) {
            LOGE("Failed to copy system files from %s to %s",
                 temp_mnt.c_str(), source.c_str());
            return false;
        }
    } else {
        if (!copy_system(source, temp_mnt)) {
            LOGE("Failed to copy system files from %s to %s",
                 source.c_str(), temp_mnt.c_str());
            return false;
        }
    }

    if (auto ret = util::umount(temp_mnt); !ret) {
        LOGE("Failed to unmount %s: %s", temp_mnt.c_str(),
             ret.error().message().c_str());
        return false;
    }

    return true;
}

/*!
 * \brief Bind mount directory or create and mount image
 *
 * \param source Source path (directory or image file)
 * \param bind_target Mount point for bind mount (if \a is_image is false)
 * \param loop_target Path to create loop device (if \a is_image is true)
 * \param is_image Whether \a source is an image file
 * \param image_size Desired image size (only used if \a is_image is true and
 *                   \a source doesn't exist)
 */
bool Installer::mount_dir_or_image(const std::string &source,
                                   const std::string &bind_target,
                                   const std::string &loop_target,
                                   bool is_image,
                                   uint64_t image_size)
{
    if (is_image) {
        struct stat sb;
        if (stat(source.c_str(), &sb) < 0) {
            double mib = static_cast<double>(image_size) / 1024 / 1024;

            display_msg("Creating image (%.1f MiB) at %s",
                        mib, source.c_str());
            display_msg("This may take a while");

            if (!create_image(source, image_size)) {
                display_msg("Failed to create image at %s", source.c_str());
                return false;
            }
        }

        auto loopdev = util::loopdev_find_unused();
        if (!loopdev) {
            LOGE("Failed to find unused loop device: %s",
                 loopdev.error().message().c_str());
            return false;
        }
        if (auto ret = util::loopdev_set_up_device(
                loopdev.value(), source, 0, false); !ret) {
            LOGE("Failed to attach %s to %s: %s",
                 loopdev.value().c_str(), source.c_str(),
                 ret.error().message().c_str());
            return false;
        }
        if (auto r = util::copy_file(loopdev.value(), loop_target, 0); !r) {
            LOGE("%s", r.error().message().c_str());
            return false;
        }

        _associated_loop_devs.push_back(std::move(loopdev.value()));
    } else {
        if (!util::mkdir_recursive(source, 0771)
                || !util::mkdir_recursive(bind_target, 0771)
                || mount(source.c_str(), bind_target.c_str(), "", MS_BIND, "") < 0) {
            display_msg("Failed to bind mount %s to %s",
                        source.c_str(), bind_target.c_str());
            return false;
        }
    }

    return true;
}

bool Installer::change_root(const std::string &path)
{
    if (unshare(CLONE_NEWNS) < 0) {
        LOGE("Failed to unshare mount namespace: %s", strerror(errno));
        return false;
    }

    if (log_mount("", "/", "", MS_PRIVATE | MS_REC, "") < 0) {
        return false;
    }

    // Unmount everything besides our chroot dir
    {
        std::vector<std::string> to_unmount;

        if (auto entries = util::get_mount_entries()) {
            for (auto const &entry : entries.value()) {
                // TODO: Use util::path_compare() instead of dumb string prefix
                //       matching
                if (entry.target != "/" && !starts_with(entry.target, path)) {
                    to_unmount.push_back(std::move(entry.target));
                }
            }
        } else {
            LOGE("Failed to get mount entries: %s",
                 entries.error().message().c_str());
            return false;
        }

        // Unmount in reverse order
        for (auto it = to_unmount.rbegin(); it != to_unmount.rend(); ++it) {
            log_umount(it->c_str());
        }
    }

    if (chdir(path.c_str()) < 0) {
        LOGE("%s: Failed to chdir: %s", path.c_str(), strerror(errno));
        return false;
    }

    if (chroot(path.c_str()) < 0) {
        LOGE("%s: Failed to chroot: %s", path.c_str(), strerror(errno));
        return false;
    }

    return true;
}

bool Installer::set_up_legacy_properties()
{
    // We don't need to worry about /dev/__properties__ since that's not present
    // in the chroot. Bionic will automatically fall back to getting the fd from
    // the ANDROID_PROPERTY_WORKSPACE environment variable.

    if (!_legacy_prop_svc.initialize()) {
        LOGE("Failed to initialize legacy property service");
        return false;
    }

    for (auto const &pair : _chroot_prop) {
        _legacy_prop_svc.set(pair.first, pair.second);
    }

    auto [fd, size] = _legacy_prop_svc.workspace();
    char tmp[32];

    snprintf(tmp, sizeof(tmp), "%d,%zu", dup(fd), size);

    char *orig_prop_env = getenv("ANDROID_PROPERTY_WORKSPACE");
    LOGD("Original properties environment: %s",
         orig_prop_env ? orig_prop_env : "(null)");

    setenv("ANDROID_PROPERTY_WORKSPACE", tmp, 1);

    LOGD("Switched to legacy properties environment: %s", tmp);

    return true;
}

bool Installer::updater_fd_reader(int stdio_fd, int command_fd)
{
    int status;
    pid_t pid = fork();

    if (pid < 0) {
        LOGE("Failed to fork updater fd reader process");
    } else if (pid == 0) {
        // Don't need command_fd in the child process
        close(command_fd);

        // Read program output in child process (stdout, stderr)
        char buf[1024];

        ScopedFILE fp(fdopen(stdio_fd, "rb"), fclose);
        if (!fp) {
            _exit(EXIT_FAILURE);
        }

        while (fgets(buf, sizeof(buf), fp.get())) {
            command_output(buf);
        }

        _exit(EXIT_SUCCESS);
    } else {
        // Read special command fd in parent process

        char buf[1024];
        char *save_ptr;

        // Similar parsing to AOSP recovery
        ScopedFILE fp(fdopen(command_fd, "rb"), fclose);

        while (fgets(buf, sizeof(buf), fp.get())) {
            char *cmd = strtok_r(buf, " \n", &save_ptr);
            if (!cmd) {
                continue;
            } else if (strcmp(cmd, "progress") == 0
                    || strcmp(cmd, "set_progress") == 0
                    || strcmp(cmd, "wipe_cache") == 0
                    || strcmp(cmd, "clear_display") == 0
                    || strcmp(cmd, "enable_reboot") == 0) {
                // Ignore
            } else if (strcmp(cmd, "ui_print") == 0) {
                char *str = strtok_r(nullptr, "\n", &save_ptr);
                if (str) {
                    updater_print(str);
                } else {
                    updater_print("\n");
                }
            } else {
                LOGE("Unknown updater command: %s", cmd);
            }
        }

        do {
            if (waitpid(pid, &status, 0) < 0) {
                LOGE("Failed to waitpid(): %s", strerror(errno));
                return false;
            }
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));

        return true;
    }

    return false;
}

/*!
 * \brief Run real update-binary in the chroot
 */
bool Installer::run_real_updater()
{
#if DEBUG_USE_ALTERNATE_UPDATER
    std::string updater(DEBUG_ALTERNATE_UPDATER_PATH);
#else
    std::string updater(_temp);
    updater += "/updater";
#endif

    std::string chroot_updater = in_chroot("/mb/updater");

    if (auto r = util::copy_file(updater, chroot_updater,
                                 util::CopyFlag::CopyAttributes
                               | util::CopyFlag::CopyXattrs); !r) {
        LOGE("%s", r.error().message().c_str());
        return false;
    }

#if DEBUG_USE_UPDATER_WRAPPER
    if (auto r = util::copy_file(DEBUG_UPDATER_WRAPPER_PATH,
                                 in_chroot("/mb/wrapper"),
                                 util::CopyFlag::CopyAttributes
                               | util::CopyFlag::CopyXattrs); !r) {
        LOGE("%s", r.error().message().c_str());
        return false;
    }
#endif

    if (chmod(chroot_updater.c_str(), 0555) < 0) {
        LOGE("%s: Failed to chmod: %s",
             chroot_updater.c_str(), strerror(errno));
        return false;
    }

    pid_t parent = getppid();

    bool aroma = is_aroma(chroot_updater);
    LOGD("update-binary is AROMA: %d", aroma);

    if (aroma) {
        kill(parent, SIGSTOP);
    }

    auto resume_aroma = finally([&]{
        if (aroma) {
            kill(parent, SIGCONT);
        }
    });


    int status;
    pid_t pid;
    int pipe_fds[2];
    int stdio_fds[2];
    if (!_passthrough) {
        if (pipe(pipe_fds) < 0) {
            return false;
        }
        if (pipe(stdio_fds) < 0) {
            close(pipe_fds[0]);
            close(pipe_fds[1]);
            return false;
        }
    }

    // Run updater in the chroot
    std::vector<std::string> argv{
#if DEBUG_USE_UPDATER_WRAPPER
        "/mb/wrapper",
#ifdef DEBUG_UPDATER_WRAPPER_ARGS
        DEBUG_UPDATER_WRAPPER_ARGS,
#endif
#endif
        "/mb/updater",
        format("%d", _interface),
        format("%d", _passthrough ? _output_fd : pipe_fds[1]),
        "/mb/install.zip"
    };

    std::vector<const char *> argv_c;
    for (const std::string &arg : argv) {
        argv_c.push_back(arg.c_str());
    }
    argv_c.push_back(nullptr);

    bool updater_ret = true;

    if ((pid = fork()) >= 0) {
        if (pid == 0) {
            if (!_passthrough) {
                // Close read ends of the pipes
                close(pipe_fds[0]);
                close(stdio_fds[0]);
            }

            if (!change_root(_chroot)) {
                _exit(EXIT_FAILURE);
            }

            if (!_passthrough) {
                if (dup2(stdio_fds[1], STDOUT_FILENO) < 0
                        || dup2(stdio_fds[1], STDERR_FILENO) < 0) {
                    _exit(EXIT_FAILURE);
                }
                close(stdio_fds[1]);
            }

            if (auto r = util::file_find_one_of(
                    "/mb/updater", {"ANDROID_PROPERTY_WORKSPACE"})) {
                LOGV("updater requires legacy properties: %d", r.value());

                if (r.value() && !set_up_legacy_properties()) {
                    _exit(EXIT_FAILURE);
                }
            } else {
                LOGE("Failed to read updater: %s", r.error().message().c_str());
                _exit(EXIT_FAILURE);
            }

            // Make sure the updater won't run interactively
            // O_CLOEXEC should not be set
            int fd_dev_null = open("/dev/null", O_RDONLY);
            if (fd_dev_null < 0) {
                LOGE("%s: Failed to open: %s", "/dev/null", strerror(errno));
                _exit(EXIT_FAILURE);
            }
            if (dup2(fd_dev_null, STDIN_FILENO) < 0) {
                LOGE("Failed to reopen stdin: %s", strerror(errno));
                _exit(EXIT_FAILURE);
            }
            close(fd_dev_null);

            // HACK: SuperSU only accepts boot partition paths that are symlinks
            setenv("BOOTIMAGE", _boot_block_dev.c_str(), 1);

            execvpe(argv_c[0], const_cast<char * const *>(argv_c.data()), environ);
            LOGE("Failed to execute updater: %s", strerror(errno));
            _exit(127);
        } else {
            if (!_passthrough) {
                // Close write ends of the pipes
                close(pipe_fds[1]);
                close(stdio_fds[1]);

                if (!updater_fd_reader(stdio_fds[0], pipe_fds[0])) {
                    LOGW("Updater fd reader process failed");
                }

                close(pipe_fds[0]);
                close(stdio_fds[0]);
            }

            do {
                if (waitpid(pid, &status, 0) < 0) {
                    LOGE("Failed to waitpid(): %s", strerror(errno));
                    return false;
                }
            } while (!WIFEXITED(status) && !WIFSIGNALED(status));

            if (WIFEXITED(status)) {
                LOGD("Updater exited with status: %d", WEXITSTATUS(status));
                if (WEXITSTATUS(status) != 0) {
                    updater_ret = false;
                }
            } else if (WIFSIGNALED(status)) {
                LOGD("Updater killed with signal: %d", WTERMSIG(status));
                updater_ret = false;
            }
        }
    }

    // Because AOSP's default updater used by just about everyone doesn't print
    // a newline at the end...
    updater_print("\n");

    if (pid < 0) {
        LOGE("Failed to execute %s: %s",
             "/mb/updater", strerror(errno));
        return false;
    }

    return updater_ret;
}

bool Installer::run_debug_shell()
{
#if !DEBUG_PRE_SHELL && !DEBUG_POST_SHELL
    return true;
#else
    int status;
    pid_t pid;

    if ((pid = fork()) >= 0) {
        if (pid == 0) {
            if (!change_root(_chroot)) {
                _exit(EXIT_FAILURE);
            }

            if (!set_up_legacy_properties()) {
                _exit(EXIT_FAILURE);
            }

            const char *argv[] = { "/sbin/sh", "-i", nullptr };
            execvpe(argv[0], const_cast<char * const *>(argv), environ);
            LOGE("Failed to execute updater: %s", strerror(errno));
            _exit(127);
        } else {
            do {
                if (waitpid(pid, &status, 0) < 0) {
                    LOGE("Failed to waitpid(): %s", strerror(errno));
                    return false;
                }
            } while (!WIFEXITED(status) && !WIFSIGNALED(status));

            return true;
        }
    }

    return false;
#endif
}

bool Installer::is_aroma(const std::string &path)
{
    auto r = util::file_find_one_of(path, {
        "AROMA Installer",
        "support@amarullz.com",
        "(c) 2013 by amarullz xda-developers",
        "META-INF/com/google/android/aroma-config",
        "META-INF/com/google/android/aroma",
        "AROMA_NAME",
        "AROMA_BUILD",
        "AROMA_VERSION"
    });
    return r && r.value();
}


/*
 * Default hooks
 */

void Installer::display_msg(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    display_msg(format_v(fmt, ap));
    va_end(ap);
}

void Installer::display_msg(std::string_view msg)
{
    printf("%.*s\n", static_cast<int>(msg.size()), msg.data());
}

// Note: Only called if we're not passing through the output_fd
void Installer::updater_print(std::string_view msg)
{
    printf("%.*s", static_cast<int>(msg.size()), msg.data());
}

// Note: Only called if we're not passing through the output_fd
void Installer::command_output(std::string_view line)
{
    printf("%.*s\n", static_cast<int>(line.size()), line.data());
}

std::unordered_map<std::string, std::string> Installer::get_properties()
{
    return std::unordered_map<std::string, std::string>();
}

Installer::ProceedState Installer::on_initialize()
{
    return ProceedState::Continue;
}

Installer::ProceedState Installer::on_created_chroot()
{
    return ProceedState::Continue;
}

Installer::ProceedState Installer::on_checked_device()
{
    return ProceedState::Continue;
}

Installer::ProceedState Installer::on_set_up_chroot()
{
    return ProceedState::Continue;
}

Installer::ProceedState Installer::on_mounted_filesystems()
{
    return ProceedState::Continue;
}

Installer::ProceedState Installer::on_pre_install()
{
    return ProceedState::Continue;
}

Installer::ProceedState Installer::on_post_install(bool install_ret)
{
    (void) install_ret;
    return ProceedState::Continue;
}

Installer::ProceedState Installer::on_unmounted_filesystems()
{
    return ProceedState::Continue;
}

Installer::ProceedState Installer::on_finished()
{
    return ProceedState::Continue;
}

void Installer::on_cleanup(Installer::ProceedState ret)
{
    (void) ret;
}


/*
 * Install stages
 */

Installer::ProceedState Installer::install_stage_initialize()
{
    LOGD("Installer version: %s (%s)", version(), git_version());

    LOGD("[Installer] Initialization stage");

    std::vector<util::ExistsInfo> info{
        { "system.transfer.list", false },
        { "system.new.dat", false },
        { "system.img", false },
        { "system.img.sparse", false },
    };
    if (!util::archive_exists(_zip_file, info)) {
        LOGE("Failed to read zip file");
    } else {
        _has_block_image = false;
        _copy_to_temp_image = false;
        for (auto const &item : info) {
            if (item.exists) {
                _has_block_image = true;
                // Flashing an Odin image discards the system image anyway, so
                // there's no point in copying the data.
                // TODO: libmbpatcher should be setting this option in info.prop
                if (item.path != "system.img.sparse") {
                    _copy_to_temp_image = true;
                }
                break;
            }
        }
    }

    return on_initialize();
}

Installer::ProceedState Installer::install_stage_create_chroot()
{
    LOGD("[Installer] Chroot creation stage");

    display_msg("Creating chroot environment");

    if (!create_chroot()) {
        display_msg("Failed to create chroot environment");
        return ProceedState::Fail;
    }


    // Post hook
    return on_created_chroot();
}

Installer::ProceedState Installer::install_stage_set_up_environment()
{
    LOGD("[Installer] Environment set up stage");

    if (!log_delete_recursive(_temp)) {
        return ProceedState::Fail;
    }
    if (log_mkdir(_temp.c_str(), 0755) < 0) {
        return ProceedState::Fail;
    }

    if (!extract_multiboot_files()) {
        display_msg("Failed to extract multiboot files from zip");
        return ProceedState::Fail;
    }

    // Load info.prop
    if (auto p = util::property_file_get_all(_temp + "/info.prop")) {
        p->swap(_prop);
    } else {
        display_msg("Failed to read multiboot/info.prop");
        return ProceedState::Fail;
    }

    return ProceedState::Continue;
}

Installer::ProceedState Installer::install_stage_check_device()
{
    LOGD("[Installer] Device verification stage");

    auto contents = util::file_read_all(_temp + "/device.json");
    if (!contents) {
        display_msg("Failed to read device.json");
        return ProceedState::Fail;
    }

    JsonError error;

    if (!device_from_json(contents.value(), _device, error)) {
        display_msg("Error when loading device.json");
        return ProceedState::Fail;
    }

    if (_device.validate()) {
        display_msg("Validation of device.json failed");
        return ProceedState::Fail;
    }

    std::string prop_product_device =
            util::property_get_string("ro.product.device", {});
    std::string prop_build_product =
            util::property_get_string("ro.build.product", {});
    std::string prop_patcher_device =
            util::property_get_string(PROP_DEVICE, {});

    LOGD("ro.product.device = %s", prop_product_device.c_str());
    LOGD("ro.build.product = %s", prop_build_product.c_str());
    LOGD(PROP_DEVICE " = %s", prop_patcher_device.c_str());
    LOGD("Target device = %s", _device.id().c_str());

    if (!prop_patcher_device.empty()) {
        _detected_device = prop_patcher_device;
    } else if (!prop_product_device.empty()) {
        _detected_device = prop_product_device;
    } else if (!prop_build_product.empty()) {
        _detected_device = prop_build_product;
    } else {
        display_msg("Failed to determine device's codename");
        return ProceedState::Fail;
    }

    // Due to optimizations in libc, strlen() may trigger valgrind errors like
    //     Address 0x4c0bf04 is 4 bytes inside a block of size 6 alloc'd
    // It's an annoyance, but not a big deal

    // Verify codename
    auto const &codenames = _device.codenames();
    auto it = std::find(codenames.begin(), codenames.end(), _detected_device);

    if (it == codenames.end()) {
        display_msg("Patched zip is for:");
        for (auto const &codename : codenames) {
            display_msg("- %s", codename.c_str());
        }
        display_msg("This device is '%s'", _detected_device.c_str());

        return ProceedState::Fail;
    }

    auto find_existing_path = [](const std::string &path) {
        return access(path.c_str(), R_OK) == 0;
    };

    auto const &boot_devs = _device.boot_block_devs();
    auto const &recovery_devs = _device.recovery_block_devs();
    auto const &system_devs = _device.system_block_devs();
    auto const &extra_devs = _device.extra_block_devs();

    // Find boot blockdev path
    it = std::find_if(boot_devs.begin(), boot_devs.end(),
                      find_existing_path);
    if (it == boot_devs.end()) {
        display_msg("Could not determine the boot block device");
        return ProceedState::Fail;
    } else {
        _boot_block_dev = *it;
        LOGD("Boot block device: %s", _boot_block_dev.c_str());
    }

    // Find recovery blockdev path
    it = std::find_if(recovery_devs.begin(), recovery_devs.end(),
                      find_existing_path);
    if (it == recovery_devs.end()) {
        LOGW("Could not determine the recovery block device");
        // Non-fatal
    } else {
        _recovery_block_dev = *it;
        LOGD("Recovery block device: %s", _recovery_block_dev.c_str());
    }

    // Find system blockdev path
    it = std::find_if(system_devs.begin(), system_devs.end(),
                      find_existing_path);
    if (it == system_devs.end()) {
        display_msg("Could not determine the system block device");
        return ProceedState::Fail;
    } else {
        _system_block_dev = *it;
        LOGD("System block device: %s", _system_block_dev.c_str());
    }

    // Other block devices to copy
    std::vector<std::string> devs;
    devs.insert(devs.end(), boot_devs.begin(), boot_devs.end());
    devs.insert(devs.end(), recovery_devs.begin(), recovery_devs.end());
    devs.insert(devs.end(), extra_devs.begin(), extra_devs.end());

    // Copy block devices to the chroot
    for (auto const &dev : devs) {
        std::string dev_path(in_chroot(dev));

        if (auto r = util::mkdir_parent(dev_path, 0755); !r) {
            LOGW("Failed to create parent directory of %s: %s",
                 dev_path.c_str(), r.error().message().c_str());
        }

        // Follow symlinks just in case the symlink source isn't in the list
        if (auto r = util::copy_file(dev, dev_path,
                                     util::CopyFlag::CopyAttributes
                                   | util::CopyFlag::CopyXattrs
                                   | util::CopyFlag::FollowSymlinks); !r) {
            LOGW("Failed to copy %s. Continuing anyway", dev.c_str());
        } else {
            LOGD("Copied %s to the chroot", dev.c_str());
        }
    }

    // Symlink CHROOT_SYSTEM_LOOP_DEV to system block devs
    for (auto const &dev : system_devs) {
        std::string dev_path(in_chroot(dev));

        if (auto r = util::mkdir_parent(dev_path, 0755); !r) {
            LOGW("Failed to create parent directory of %s: %s",
                 dev_path.c_str(), r.error().message().c_str());
        }

        if (symlink(CHROOT_SYSTEM_LOOP_DEV, dev_path.c_str()) < 0) {
            LOGW("Failed to symlink %s to %s: %s. Continuing anyway",
                 CHROOT_SYSTEM_LOOP_DEV, dev_path.c_str(), strerror(errno));
        } else {
            LOGD("Symlinked %s to %s",
                 CHROOT_SYSTEM_LOOP_DEV, dev_path.c_str());
        }
    }

    return on_checked_device();
}

Installer::ProceedState Installer::install_stage_get_install_type()
{
    LOGD("[Installer] Retrieve install type stage");

    std::string install_type = get_install_type();

    if (install_type == CANCELLED) {
        display_msg("Cancelled installation");
        return ProceedState::Cancel;
    }

    _rom = Roms::create_rom(install_type);
    if (!_rom) {
        display_msg("Unknown ROM ID: %s", install_type.c_str());
        return ProceedState::Fail;
    }

    display_msg("ROM ID: " + _rom->id);

    if (_rom->id == "primary") {
        _copy_to_temp_image = true;
    }

    _system_path = _rom->full_system_path();
    _cache_path = _rom->full_cache_path();
    _data_path = _rom->full_data_path();

    if (_system_path.empty()) {
        display_msg("Failed to determine system path");
        return ProceedState::Fail;
    } else if (_cache_path.empty()) {
        display_msg("Failed to determine cache path");
        return ProceedState::Fail;
    } else if (_data_path.empty()) {
        display_msg("Failed to determine data path");
        return ProceedState::Fail;
    }

    display_msg("- /system: " + _system_path);
    display_msg("- /cache: " + _cache_path);
    display_msg("- /data: " + _data_path);
    display_msg("- System is image file: %s",
                _rom->system_is_image ? "true" : "false");
    display_msg("- Cache is image file: %s",
                _rom->cache_is_image ? "true" : "false");
    display_msg("- Data is image file: %s",
                _rom->data_is_image ? "true" : "false");
    LOGV("ROM ID: %s", _rom->id.c_str());

    return ProceedState::Continue;
}

Installer::ProceedState Installer::install_stage_set_up_chroot()
{
    LOGD("[Installer] Chroot set up stage");

    // Save a copy of the boot image that we'll restore if the installation fails
    if (auto r = util::copy_contents(
            _boot_block_dev, _temp + "/boot.orig"); !r) {
        LOGE("%s", r.error().message().c_str());
        display_msg("Failed to back up boot partition");
        return ProceedState::Fail;
    }

    // Switch to target ROM if possible
    std::string boot_image_path(_rom->boot_image_path());
    if (access(boot_image_path.c_str(), R_OK) == 0) {
        // Use an empty base dirs list since we don't want to flash any non-boot
        // partitions
        auto result = switch_rom(_rom->id, _boot_block_dev, {}, true);
        if (result != SwitchRomResult::Succeeded) {
            display_msg("Failed to switch to target ROM. Continuing anyway...");
            LOGW("Failed to switch to target ROM: %d",
                 static_cast<int>(result));
        } else {
            LOGV("Successfully switched to target ROM");
        }
    } else {
        LOGV("Won't switch to non-existent target ROM");
    }

    // Patch current boot image to undo the init symlink patch. This is
    // necessary to flash something that also tries to touch /init.
    LOGV("Patching ramdisk to undo init modifications");

    std::string temp_boot_img(_temp);
    temp_boot_img += "/boot.img";

    if (InstallerUtil::patch_boot_image(_boot_block_dev, temp_boot_img, {
        rp_restore_init(),
    })) {
        if (auto r = util::copy_contents(temp_boot_img, _boot_block_dev); !r) {
            LOGW("Failed to copy %s to %s: %s. Continuing anyway...",
                 temp_boot_img.c_str(), _boot_block_dev.c_str(),
                 r.error().message().c_str());
        }
    } else {
        LOGW("Failed to patch boot image. Continuing anyway...");
    }

    // Wrap busybox to disable some applets
    if (!set_up_busybox_wrapper()) {
        display_msg("Failed to extract busybox wrapper");
        return ProceedState::Fail;
    }

    // Copy ourself for the real update-binary to use
    (void) util::copy_file(_temp + "/mbtool", in_chroot(HELPER_TOOL),
                           util::CopyFlag::CopyAttributes
                         | util::CopyFlag::CopyXattrs);
    chmod(in_chroot(HELPER_TOOL).c_str(), 0555);

    // Copy /default.prop
    (void) util::copy_file("/default.prop", in_chroot("/default.prop"),
                           util::CopyFlag::CopyAttributes
                         | util::CopyFlag::CopyXattrs);

    // Copy file_contexts
    (void) util::copy_file("/file_contexts", in_chroot("/file_contexts"),
                           util::CopyFlag::CopyAttributes
                         | util::CopyFlag::CopyXattrs);

    return on_set_up_chroot();
}

Installer::ProceedState Installer::install_stage_mount_filesystems()
{
    LOGD("[Installer] Filesystem mounting stage");

    if (_flags & InstallerFlag::SkipMountingVolumes) {
        LOGV("Skipping filesystem mounting stage");
        return ProceedState::Continue;
    }

    if (!mount_dir_or_image(_cache_path,
                            in_chroot(CHROOT_CACHE_BIND_MOUNT),
                            in_chroot(CHROOT_CACHE_LOOP_DEV),
                            _rom->cache_is_image, DEFAULT_IMAGE_SIZE)) {
        return ProceedState::Fail;
    }

    // Because CM 12.1 requires this and silently dies (ie. completes
    // "successfully") if it doesn't exist
    if (log_mkdir(in_chroot("/cache/recovery").c_str(), 0755) < 0
            && errno != EEXIST) {
        LOGE("Failed to create /cache/recovery: %s", strerror(errno));
        display_msg("Failed to create /cache/recovery in chroot");
        return ProceedState::Fail;
    }

    if (!mount_dir_or_image(_data_path,
                            in_chroot(CHROOT_DATA_BIND_MOUNT),
                            in_chroot(CHROOT_DATA_LOOP_DEV),
                            _rom->data_is_image, DEFAULT_IMAGE_SIZE)) {
        return ProceedState::Fail;
    }

    // Get desired system image size
    auto system_size = util::get_blockdev_size(_system_block_dev);
    if (!system_size) {
        display_msg("Failed to get size of system partition");
        display_msg("Image size will be 4 GiB");
        system_size = DEFAULT_IMAGE_SIZE;
    }

    bool system_is_image = _rom->system_is_image;
    std::string system_path = _system_path;

    // Create temporary system image if needed
    if (!_rom->system_is_image && (_has_block_image || _rom->id == "primary")) {
        // Try /data/.system.img.tmp and if /data doesn't have enough space,
        // then try [External SD]/.system.img.tmp

        _temp_image_path = Roms::get_data_partition();
        _temp_image_path += "/.system.img.tmp";
        remove(_temp_image_path.c_str());

        if (!create_image(_temp_image_path, system_size.value())) {
            display_msg("Failed to create temporary image %s",
                        _temp_image_path.c_str());

            // Try external SD
            std::string mountpoint(Roms::get_extsd_partition());
            if (!mountpoint.empty()) {
                display_msg("Trying to create temporary image on external SD");
                display_msg("(This will be slow)");

                _temp_image_path = std::move(mountpoint);
                _temp_image_path += "/.system.img.tmp";
                remove(_temp_image_path.c_str());

                if (!create_image(_temp_image_path, system_size.value())) {
                    return ProceedState::Fail;
                }
            } else {
                return ProceedState::Fail;
            }
        }

        if (_copy_to_temp_image) {
            display_msg("Copying system to temporary image");

            // Copy current /system files to the image
            if (!system_image_copy(_system_path, _temp_image_path, false)) {
                display_msg("Failed to copy %s to %s",
                            _system_path.c_str(), _temp_image_path.c_str());
                return ProceedState::Fail;
            }
        }

        system_is_image = true;
        system_path = _temp_image_path;
    }

    if (!mount_dir_or_image(system_path,
                            in_chroot(CHROOT_SYSTEM_BIND_MOUNT),
                            in_chroot(CHROOT_SYSTEM_LOOP_DEV),
                            system_is_image, system_size.value())) {
        return ProceedState::Fail;
    }

    // Bind-mount zip file
    (void) util::create_empty_file(in_chroot("/mb/install.zip"));
    if (log_mount(_zip_file.c_str(), in_chroot("/mb/install.zip").c_str(),
                  "", MS_BIND, "") < 0) {
        return ProceedState::Fail;
    }

    return on_mounted_filesystems();
}

Installer::ProceedState Installer::install_stage_installation()
{
    using namespace std::chrono;

    LOGD("[Installer] Installation stage");

    ProceedState hook_ret = on_pre_install();
    if (hook_ret != ProceedState::Continue) return hook_ret;

    // Get chroot props
    _chroot_prop = get_properties();

    // Run real update-binary
    display_msg("Running real update-binary");
    display_msg("Here we go!");

#if DEBUG_PRE_SHELL
    LOGD("To skip installation, create a file named: /.skip-install");
    LOGD("Pre-installation shell");
    run_debug_shell();
#endif

    bool updater_ret = true;

    struct stat sb;
    if (lstat(in_chroot("/.skip-install").c_str(), &sb) < 0
            && errno == ENOENT) {
        auto start = steady_clock::now();
        updater_ret = run_real_updater();
        auto stop = steady_clock::now();
        auto ms = duration_cast<milliseconds>(stop - start);

        auto h = duration_cast<hours>(ms);
        ms -= h;
        auto m = duration_cast<minutes>(ms);
        ms -= m;
        auto s = duration_cast<seconds>(ms);
        ms -= s;

        display_msg("Elapsed time: %02" PRIu64 ":%02" PRIu64 ":%02" PRIu64
                    ".%03" PRIu64, static_cast<uint64_t>(h.count()),
                    static_cast<uint64_t>(m.count()),
                    static_cast<uint64_t>(s.count()),
                    static_cast<uint64_t>(ms.count()));

        if (!updater_ret) {
            display_msg("Failed to run real update-binary");
        }
    } else {
        LOGD("Skipping installation as requested");
    }

#if DEBUG_POST_SHELL
    LOGD("Post-installation shell");
    run_debug_shell();
#endif

    run_command_chroot(_chroot, { HELPER_TOOL, "mount", "/system" });

    // Grab version and display ID so that can be cached in config.json later
    if (auto props = util::property_file_get_all(
            in_chroot("/system/build.prop"))) {
        auto to_cache = {
            "ro.build.version.release",
            "ro.build.display.id",
        };

        for (auto const &prop : to_cache) {
            if (auto it = props->find(prop); it != props->end()) {
                _cached_prop[it->first] = it->second;
            }
        }
    }

    // Determine if fuse-exfat should be used. We can't detect this at boot time
    // since vold is not yet available. We used to detect whether fuse-exfat
    // should be used by checking for the presence of "EXFAT   " or "exfat" in
    // /init. That would work because only ROMs that use fuse-exfat link init to
    // libblkid. However, there were exceptions to that, such as with the Moto
    // X Pure, which uses the exfat kernel module in all ROMs. This is a more
    // robust solution since the "/system/bin/mount.exfat" string only exists
    // #ifndef CONFIG_KERNEL_HAVE_EXFAT
    if (auto r = util::file_find_one_of(
            in_chroot("/system/bin/vold"), {"/system/bin/mount.exfat"});
            r && r.value()) {
        _use_fuse_exfat = true;
    }

    hook_ret = on_post_install(updater_ret);
    if (hook_ret != ProceedState::Continue) return hook_ret;

    return updater_ret ? ProceedState::Continue : ProceedState::Fail;
}

Installer::ProceedState Installer::install_stage_unmount_filesystems()
{
    LOGD("[Installer] Filesystem unmounting stage");

    // Umount filesystems from inside the chroot
    run_command_chroot(_chroot, { HELPER_TOOL, "unmount", "/system" });
    run_command_chroot(_chroot, { HELPER_TOOL, "unmount", "/cache" });
    run_command_chroot(_chroot, { HELPER_TOOL, "unmount", "/data" });

    // Disassociate loop devices
    for (const std::string &loop_dev : _associated_loop_devs) {
        if (auto ret = util::loopdev_remove_device(loop_dev); !ret) {
            LOGE("%s: Failed to disassociate loop device: %s",
                 loop_dev.c_str(), ret.error().message().c_str());
        }
    }

    if (_rom->cache_is_image && !util::umount(in_chroot("/cache"))) {
        display_msg("Failed to unmount %s", in_chroot("/cache").c_str());
    }

    if (_rom->data_is_image && !util::umount(in_chroot("/data"))) {
        display_msg("Failed to unmount %s", in_chroot("/data").c_str());
    }

    if (_rom->system_is_image) {
        // Run file system checks
        if (!fsck_ext4_image(_system_path)) {
            display_msg("Failed to run e2fsck on image");
        }
    } else {
        if (!(_flags & InstallerFlag::SkipMountingVolumes)
                && (_has_block_image || _rom->id == "primary")) {
            display_msg("Copying temporary image to system");

            // Format system directory
            if (!wipe_directory(_system_path, {})) {
                display_msg("Failed to wipe %s", _system_path.c_str());
                return ProceedState::Fail;
            }

            // Copy image back to system directory
            if (!system_image_copy(_system_path, _temp_image_path, true)) {
                display_msg("Failed to copy %s to %s",
                            _temp_image_path.c_str(), _system_path.c_str());
                return ProceedState::Fail;
            }
        }
    }


    return on_unmounted_filesystems();
}

Installer::ProceedState Installer::install_stage_finish()
{
    LOGD("[Installer] Finalization stage");

    display_msg("Patching boot image");

    std::string temp_boot_img(_temp);
    temp_boot_img += "/boot.img";

    if (!InstallerUtil::patch_boot_image(_boot_block_dev, temp_boot_img, {
        rp_write_rom_id(_rom->id),
        rp_restore_default_prop(),
        rp_add_dbp_prop(_detected_device, _use_fuse_exfat),
        rp_add_binaries(_temp + "/binaries"),
        rp_symlink_fuse_exfat(),
        rp_symlink_init(),
        rp_add_device_json(_temp + "/device.json"),
    })) {
        display_msg("Failed to patch boot image");
        return ProceedState::Fail;
    }

    // Write to multiboot directory and boot partition

    std::string path(MULTIBOOT_DIR);
    path += "/";
    path += _rom->id;
    path += "/boot.img";
    if (!util::mkdir_parent(path, 0775)) {
        display_msg("Failed to create %s", path.c_str());
        return ProceedState::Fail;
    }

    if (auto r = util::copy_contents(temp_boot_img, _boot_block_dev); !r) {
        LOGE("Failed to copy %s to %s: %s", temp_boot_img.c_str(),
             _boot_block_dev.c_str(), r.error().message().c_str());
        display_msg("Failed to flash patched boot image");
        return ProceedState::Fail;
    }
    if (auto r = util::copy_contents(temp_boot_img, path); !r) {
        LOGE("Failed to copy %s to %s: %s", temp_boot_img.c_str(), path.c_str(),
             r.error().message().c_str());
        display_msg("Failed to back up boot image");
        return ProceedState::Fail;
    }

    // Update checksums
    auto digest = util::sha512_hash(temp_boot_img);
    if (!digest) {
        LOGE("%s: Failed to compute sha512sum: %s",
             temp_boot_img.c_str(), digest.error().message().c_str());
        display_msg("Failed to compute sha512sum of new boot image");
        return ProceedState::Fail;
    }

    std::string hash = util::hex_string(digest.value().data(),
                                        digest.value().size());

    ChecksumProps props;
    props.load_file();
    props.set(_rom->id, "boot.img", hash);
    props.save_file();

    // Write cached properties to config
    std::string config_path(_rom->config_path());
    RomConfig config;
    if (config.load_file(config_path)) {
        config.cached_props = _cached_prop;
        if (!config.save_file(config_path)) {
            LOGW("%s: Failed to save config", config_path.c_str());
        }
    } else {
        LOGW("%s: Failed to load config", config_path.c_str());
    }

    fix_multiboot_permissions();

    return on_finished();
}

void Installer::install_stage_cleanup(Installer::ProceedState ret)
{
    LOGD("[Installer] Cleanup stage");

    if (ret == ProceedState::Fail) {
        display_msg("Failed to flash zip file.");
    }

    display_msg("Destroying chroot environment");

    remove(_temp_image_path.c_str());

    if (ret == ProceedState::Fail && !_boot_block_dev.empty()) {
        if (auto r = util::copy_contents(
                _temp + "/boot.orig", _boot_block_dev); !r) {
            LOGE("Failed to restore boot partition: %s",
                 r.error().message().c_str());
            display_msg("Failed to restore boot partition");
        }
    }

    if (!destroy_chroot()) {
        display_msg("Failed to destroy chroot environment. You should "
                    "reboot into recovery again to avoid flashing issues.");
    }

    on_cleanup(ret);

    LOGV("Finished cleanup");
}

bool Installer::start_installation()
{
    if (_ran) {
        LOGE("Installation already executed");
        return false;
    } else {
        _ran = true;
    }

    ProceedState ret = ProceedState::Fail;

    auto when_finished = finally([&] {
        install_stage_cleanup(ret);
    });


    ret = install_stage_initialize();
    if (ret == ProceedState::Fail) return false;
    else if (ret == ProceedState::Cancel) return true;

    ret = install_stage_create_chroot();
    if (ret == ProceedState::Fail) return false;
    else if (ret == ProceedState::Cancel) return true;

    ret = install_stage_set_up_environment();
    if (ret == ProceedState::Fail) return false;
    else if (ret == ProceedState::Cancel) return true;

    ret = install_stage_check_device();
    if (ret == ProceedState::Fail) return false;
    else if (ret == ProceedState::Cancel) return true;

    ret = install_stage_get_install_type();
    if (ret == ProceedState::Fail) return false;
    else if (ret == ProceedState::Cancel) return true;

    ret = install_stage_set_up_chroot();
    if (ret == ProceedState::Fail) return false;
    else if (ret == ProceedState::Cancel) return true;

    ret = install_stage_mount_filesystems();
    if (ret == ProceedState::Fail) return false;
    else if (ret == ProceedState::Cancel) return true;

    ProceedState install_ret = install_stage_installation();

    ret = install_stage_unmount_filesystems();
    if (ret == ProceedState::Fail) return false;
    else if (ret == ProceedState::Cancel) return true;

    ret = install_ret;
    if (ret == ProceedState::Fail) return false;
    else if (ret == ProceedState::Cancel) return true;

    ret = install_stage_finish();
    if (ret == ProceedState::Fail) return false;
    else if (ret == ProceedState::Cancel) return true;

    return true;
}

}
