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

#include "installer.h"

// Linux/posix
#include <fcntl.h>
#include <signal.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <unistd.h>

// libmbp
#include <libmbp/bootimage.h>
#include <libmbp/patcherconfig.h>

// Local
#include "lokipatch.h"
#include "main.h"
#include "multiboot.h"
#include "util/archive.h"
#include "util/chown.h"
#include "util/command.h"
#include "util/copy.h"
#include "util/delete.h"
#include "util/directory.h"
#include "util/file.h"
#include "util/finally.h"
#include "util/logging.h"
#include "util/loopdev.h"
#include "util/mount.h"
#include "util/properties.h"
#include "util/string.h"


// Set to 1 to spawn a shell after installation
// NOTE: This should ONLY be used through adb. For example:
//
//     $ adb push mbtool_recovery /tmp/updater
//     $ adb shell /tmp/updater 3 1 /path/to/file_patched.zip
#define DEBUG_SHELL 0

// Use an update-binary file not from the zip file
#define DEBUG_USE_ALTERNATE_UPDATER 0
#define DEBUG_ALTERNATE_UPDATER_PATH "/tmp/updater.orig"


namespace mb {

const std::string Installer::HELPER_TOOL = "/update-binary-tool";
const std::string Installer::UPDATE_BINARY =
        "META-INF/com/google/android/update-binary";
const std::string Installer::MULTIBOOT_UNZIP = "multiboot/unzip";
const std::string Installer::MULTIBOOT_AROMA = "multiboot/aromawrapper.zip";
const std::string Installer::MULTIBOOT_BBWRAPPER = "multiboot/bb-wrapper.sh";
const std::string Installer::MULTIBOOT_DEVICE = "multiboot/device";
const std::string Installer::TEMP_SYSTEM_IMAGE = "/data/.system.img.tmp";
const std::string Installer::CANCELLED = "cancelled";


Installer::Installer(std::string zip_file, std::string chroot_dir,
                     std::string temp_dir, int interface, int output_fd) :
    _zip_file(std::move(zip_file)),
    _chroot(std::move(chroot_dir)),
    _temp(std::move(temp_dir)),
    _interface(interface),
    _output_fd(output_fd)
{
}

Installer::~Installer()
{
}


/*
 * Wrappers around functions that log failures
 */

int log_mkdir(const char *pathname, mode_t mode)
{
    if (mkdir(pathname, mode) < 0) {
        LOGE("Failed to create {}: {}", pathname, strerror(errno));
        return false;
    }
    return true;
}

int log_mount(const char *source, const char *target, const char *fstype,
              long unsigned int mountflags, const void *data)
{
    if (mount(source, target, fstype, mountflags, data) < 0) {
        LOGE("Failed to mount {} ({}) at {}: {}",
             source, fstype, target, strerror(errno));
        return false;
    }
    return true;
}

int log_mknod(const char *pathname, mode_t mode, dev_t dev)
{
    if (mknod(pathname, mode, dev) < 0) {
        LOGE("Failed to create special file {}: {}", pathname, strerror(errno));
        return false;
    }
    return true;
}

bool log_is_mounted(const std::string &mountpoint)
{
    if (!util::is_mounted(mountpoint)) {
        LOGE("{} is not mounted", mountpoint);
        return false;
    }
    return true;
}

bool log_unmount_all(const std::string &dir)
{
    if (!util::unmount_all(dir)) {
        LOGE("Failed to unmount all mountpoints within {}", dir);
        return false;
    }
    return true;
}

bool log_delete_recursive(const std::string &path)
{
    if (!util::delete_recursive(path)) {
        LOGE("Failed to recursively remove {}", path);
        return false;
    }
    return true;
}

bool log_copy_dir(const std::string &source,
                  const std::string &target, int flags)
{
    if (!util::copy_dir(source, target, flags)) {
        LOGE("Failed to copy contents of {}/ to {}/", source, target);
        return false;
    }
    return true;
}


/*
 * Helper functions
 */

std::string Installer::in_chroot(const std::string &path) const
{
    if (!path.empty() && path[0] == '/') {
        return std::string(_chroot).append(path);
    } else {
        return std::string(_chroot).append("/").append(path);
    }
}

bool Installer::create_chroot() const
{
    // We'll just call the recovery's mount tools directly to avoid having to
    // parse TWRP and CWM's different fstab formats
    util::run_command({ "mount", "/system" });
    util::run_command({ "mount", "/cache" });
    util::run_command({ "mount", "/data" });

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

    // Set up directories
    if (log_mkdir(_chroot.c_str(), 0755) < 0
            || log_mkdir(in_chroot("/mb").c_str(), 0755) < 0
            || log_mkdir(in_chroot("/dev").c_str(), 0755) < 0
            || log_mkdir(in_chroot("/etc").c_str(), 0755) < 0
            || log_mkdir(in_chroot("/proc").c_str(), 0755) < 0
            || log_mkdir(in_chroot("/sbin").c_str(), 0755) < 0
            || log_mkdir(in_chroot("/sys").c_str(), 0755) < 0
            || log_mkdir(in_chroot("/tmp").c_str(), 0755) < 0
            || log_mkdir(in_chroot("/data").c_str(), 0755) < 0
            || log_mkdir(in_chroot("/cache").c_str(), 0755) < 0
            || log_mkdir(in_chroot("/system").c_str(), 0755) < 0) {
        return false;
    }

    // Other mounts
    if (log_mount("none", in_chroot("/dev").c_str(), "tmpfs", 0, "") < 0
            || log_mkdir(in_chroot("/dev/pts").c_str(), 0755) < 0
            || log_mount("none", in_chroot("/dev/pts").c_str(), "devpts", 0, "") < 0
            || log_mount("none", in_chroot("/proc").c_str(), "proc", 0, "") < 0
            || log_mount("none", in_chroot("/sbin").c_str(), "tmpfs", 0, "") < 0
            || log_mount("none", in_chroot("/sys").c_str(), "sysfs", 0, "") < 0
            || log_mount("none", in_chroot("/tmp").c_str(), "tmpfs", 0, "") < 0) {
        return false;
    }

    // Some recoveries don't have SELinux enabled
    if (log_mount("none", in_chroot("/sys/fs/selinux").c_str(), "selinuxfs", 0, "") < 0
            && errno != ENOENT) {
        LOGV("Ignoring /sys/fs/selinux mount");
        LOGE("Failed to mount {} ({}) at {}: {}",
             "none", "selinuxfs", in_chroot("/sys/fs/selinux"), strerror(errno));
        return false;
    }

    // Copy the contents of sbin since we need to mess with some of the binaries
    // there. Also, for whatever reason, bind mounting /sbin results in EINVAL
    // no matter if it's done from here or from busybox.
    if (!log_copy_dir("/sbin", in_chroot("/sbin"),
                      util::MB_COPY_ATTRIBUTES
                    | util::MB_COPY_XATTRS
                    | util::MB_COPY_EXCLUDE_TOP_LEVEL)) {
        return false;
    }

    // Don't create unnecessary special files in /dev to avoid install scripts
    // from overwriting partitions
    if (log_mknod(in_chroot("/dev/console").c_str(), S_IFCHR | 0644, makedev(5, 1)) < 0
            || log_mknod(in_chroot("/dev/null").c_str(), S_IFCHR | 0644, makedev(1, 3)) < 0
            || log_mknod(in_chroot("/dev/ptmx").c_str(), S_IFCHR | 0644, makedev(5, 2)) < 0
            || log_mknod(in_chroot("/dev/random").c_str(), S_IFCHR | 0644, makedev(1, 8)) < 0
            || log_mknod(in_chroot("/dev/tty").c_str(), S_IFCHR | 0644, makedev(5, 0)) < 0
            || log_mknod(in_chroot("/dev/urandom").c_str(), S_IFCHR | 0644, makedev(1, 9)) < 0
            || log_mknod(in_chroot("/dev/zero").c_str(), S_IFCHR | 0644, makedev(1, 5)) < 0
            || log_mknod(in_chroot("/dev/loop-control").c_str(), S_IFCHR | 0644, makedev(10, 237)) < 0) {
        return false;
    }

    // Create a few loopback devices in case we need to use them
    if (log_mkdir(in_chroot("/dev/block").c_str(), 0755) < 0
            || log_mknod(in_chroot("/dev/block/loop0").c_str(), S_IFBLK | 0644, makedev(7, 0)) < 0
            || log_mknod(in_chroot("/dev/block/loop1").c_str(), S_IFBLK | 0644, makedev(7, 1)) < 0
            || log_mknod(in_chroot("/dev/block/loop2").c_str(), S_IFBLK | 0644, makedev(7, 2)) < 0) {
        return false;
    }

    // We need /dev/input/* and /dev/graphics/* for AROMA
#if 1
    if (!log_copy_dir("/dev/input", in_chroot("/dev/input"),
                      util::MB_COPY_ATTRIBUTES
                    | util::MB_COPY_XATTRS
                    | util::MB_COPY_EXCLUDE_TOP_LEVEL)) {
        return false;
    }
    if (!log_copy_dir("/dev/graphics", in_chroot("/dev/graphics"),
                      util::MB_COPY_ATTRIBUTES
                    | util::MB_COPY_XATTRS
                    | util::MB_COPY_EXCLUDE_TOP_LEVEL)) {
        return false;
    }
#else
    if (log_mkdir(in_chroot("/dev/input"), 0755) < 0
            || log_mkdir(in_chroot("/dev/graphics"), 0755) < 0
            || log_mount("/dev/input", in_chroot("/dev/input"), "", MS_BIND, "") < 0
            || log_mount("/dev/graphics", in_chroot("/dev/graphics"), "", MS_BIND, "") < 0) {
        return false;
    }
#endif

    util::create_empty_file(in_chroot("/.chroot"));

    return true;
}

bool Installer::destroy_chroot() const
{
    umount(in_chroot("/system").c_str());
    umount(in_chroot("/cache").c_str());
    umount(in_chroot("/data").c_str());

    umount(in_chroot("/mb/system.img").c_str());

    umount(in_chroot("/dev/pts").c_str());
    umount(in_chroot("/dev").c_str());
    umount(in_chroot("/proc").c_str());
    umount(in_chroot("/sys/fs/selinux").c_str());
    umount(in_chroot("/sys").c_str());
    umount(in_chroot("/tmp").c_str());
    umount(in_chroot("/sbin").c_str());

    // Unmount everything previously mounted in the chroot
    if (!util::unmount_all(_chroot)) {
        LOGE("Failed to unmount previous mount points in {}", _chroot);
        return false;
    }

    util::delete_recursive(_chroot);

    return true;
}

/*!
 * \brief Extract needed multiboot files from the patched zip file
 */
bool Installer::extract_multiboot_files()
{
    std::vector<util::extract_info> files{
        { UPDATE_BINARY + ".orig",  _temp + "/updater"          },
        { MULTIBOOT_UNZIP,          _temp + "/unzip"            },
        { MULTIBOOT_AROMA,          _temp + "/aromawrapper.zip" },
        { MULTIBOOT_BBWRAPPER,      _temp + "/bb-wrapper.sh"    },
        { MULTIBOOT_DEVICE,         _temp + "/device"           }
    };

    if (!util::extract_files2(_zip_file, files)) {
        LOGE("Failed to extract all multiboot files");
        return false;
    }

    return true;
}

/*!
 * \brief Replace /sbin/unzip in the chroot with one supporting zip flags 1 & 8
 */
bool Installer::set_up_unzip()
{
    std::string temp_unzip = _temp + "/unzip";
    std::string sbin_unzip = in_chroot("/sbin/unzip");

    remove(sbin_unzip.c_str());

    if (!util::copy_file(temp_unzip, sbin_unzip,
                         util::MB_COPY_ATTRIBUTES | util::MB_COPY_XATTRS)) {
        LOGE("Failed to copy {} to {}: {}",
             temp_unzip, sbin_unzip, strerror(errno));
        return false;
    }

    if (chmod(sbin_unzip.c_str(), 0555) < 0) {
        LOGE("Failed to chmod {}: {}", sbin_unzip, strerror(errno));
        return false;
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

    if (!util::copy_file(temp_busybox, sbin_busybox,
                         util::MB_COPY_ATTRIBUTES | util::MB_COPY_XATTRS)) {
        LOGE("Failed to copy {} to {}: {}",
             temp_busybox, sbin_busybox, strerror(errno));
        return false;
    }

    if (chmod(sbin_busybox.c_str(), 0555) < 0) {
        LOGE("Failed to chmod {}: {}", sbin_busybox, strerror(errno));
        return false;
    }

    return true;
}

/*!
 * \brief Create 3G temporary ext4 image
 *
 * \param path Image file path
 */
bool Installer::create_temporary_image(const std::string &path)
{
    static const char *image_size = "3G";

    remove(path.c_str());

    if (!util::mkdir_parent(path, S_IRWXU)) {
        LOGE("{}: Failed to create parent directory: {}",
             path, strerror(errno));
        return false;
    }

    struct stat sb;
    if (stat(path.c_str(), &sb) < 0) {
        if (errno != ENOENT) {
            LOGE("{}: Failed to stat: {}", path, strerror(errno));
            return false;
        } else {
            LOGD("{}: Creating new {} ext4 image", path, image_size);

            // Create new image
            if (util::run_command(
                    { "make_ext4fs", "-l", image_size, path }) != 0) {
                LOGE("{}: Failed to create image", path);
                return false;
            }
            return true;
        }
    }

    LOGE("{}: File already exists", path);
    return false;
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
    std::string temp_image(_temp);
    temp_image += "/.system.tmp";

    struct stat sb;
    std::string loopdev;

    auto done = util::finally([&] {
        if (!loopdev.empty()) {
            umount(temp_image.c_str());
            util::loopdev_remove_device(loopdev);
        }
    });

    if (stat(source.c_str(), &sb) < 0
            && !util::mkdir_recursive(source, 0755)) {
        LOGE("Failed to create {}: {}", source, strerror(errno));
        return false;
    }

    if (stat(temp_image.c_str(), &sb) < 0
            && mkdir(temp_image.c_str(), 0755) < 0) {
        LOGE("Failed to create {}: {}", temp_image, strerror(errno));
        return false;
    }

    loopdev = util::loopdev_find_unused();
    if (loopdev.empty()) {
        LOGE("Failed to find unused loop device: {}", strerror(errno));
        return false;
    }

    if (!util::loopdev_set_up_device(loopdev, image, 0, 0)) {
        LOGE("Failed to set up loop device {}: {}", loopdev, strerror(errno));
        return false;
    }

    if (mount(loopdev.c_str(), temp_image.c_str(), "ext4", 0, "") < 0) {
        LOGE("Failed to mount {}: {}", loopdev, strerror(errno));
        return false;
    }

    if (reverse) {
        if (!copy_system(temp_image, source)) {
            LOGE("Failed to copy system files from {} to {}",
                 temp_image, source);
            return false;
        }
    } else {
        if (!copy_system(source, temp_image)) {
            LOGE("Failed to copy system files from {} to {}",
                 source, temp_image);
            return false;
        }
    }

    if (umount(temp_image.c_str()) < 0) {
        LOGE("Failed to unmount {}: {}", temp_image, strerror(errno));
        return false;
    }

    if (!util::loopdev_remove_device(loopdev)) {
        LOGE("Failed to remove loop device {}: {}", loopdev, strerror(errno));
        return false;
    }

    return true;
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

    if (!util::copy_file(updater, chroot_updater,
                         util::MB_COPY_ATTRIBUTES | util::MB_COPY_XATTRS)) {
        LOGE("Failed to copy {} to {}: {}",
             updater, chroot_updater, strerror(errno));
        return false;
    }

    if (chmod(chroot_updater.c_str(), 0555) < 0) {
        LOGE("{}: Failed to chmod: {}",
             chroot_updater, strerror(errno));
        return false;
    }

    std::vector<std::string> argv{
        "/mb/updater",
        util::to_string(_interface),
        util::to_string(_output_fd),
        "/mb/install.zip"
    };

    pid_t parent = getppid();

    bool aroma = is_aroma(chroot_updater);
    LOGD("update-binary is AROMA: {}", aroma);

    if (aroma) {
        kill(parent, SIGSTOP);
    }

    int ret;
    if ((ret = util::run_command_chroot(_chroot, argv)) != 0) {
        if (aroma) {
            kill(parent, SIGCONT);
        }

        if (ret < 0) {
            LOGE("Failed to execute {}: {}",
                 "/mb/updater", strerror(errno));
        } else {
            LOGE("{} returned non-zero exit status",
                 "/mb/updater");
        }
        return false;
    }

    if (aroma) {
        kill(parent, SIGCONT);
    }

    return true;
}

bool Installer::is_aroma(const std::string &path)
{
    return util::file_find_one_of(path, {
        "AROMA Installer",
        "support@amarullz.com",
        "(c) 2013 by amarullz xda-developers",
        "META-INF/com/google/android/aroma-config",
        "META-INF/com/google/android/aroma",
        "AROMA_NAME",
        "AROMA_BUILD",
        "AROMA_VERSION"
    });
}


/*
 * Default hooks
 */

void Installer::display_msg(const std::string &msg)
{
    printf("%s\n", msg.c_str());
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
    LOGD("[Installer] Initialization stage");

    std::vector<util::exists_info> info{{ "system.transfer.list", false }};
    if (!util::archive_exists(_zip_file, info)) {
        LOGE("Failed to read zip file");
    } else {
        _has_block_image = info[0].exists;
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

    return ProceedState::Continue;
}

Installer::ProceedState Installer::install_stage_check_device()
{
    LOGD("[Installer] Device verification stage");

    mbp::PatcherConfig pc;
    LOGD("libmbp-mini version: {}", pc.version());

    std::string prop_product_device;
    std::string prop_build_product;

    // Verify device
    if (!util::file_first_line(_temp + "/device", &_device)) {
        LOGE("Failed to read {}/device", _temp);
        display_msg("Failed to determine target device");
        return ProceedState::Fail;
    }

    util::get_property("ro.product.device", &prop_product_device, "");
    util::get_property("ro.build.product", &prop_build_product, "");

    LOGD("ro.product.device = {}", prop_product_device);
    LOGD("ro.build.product = {}", prop_build_product);
    LOGD("Target device = {}", _device);


    // Due to optimizations in libc, strlen() may trigger valgrind errors like
    //     Address 0x4c0bf04 is 4 bytes inside a block of size 6 alloc'd
    // It's an annoyance, but not a big deal

    bool found_device = false;

    for (const mbp::Device *d : pc.devices()) {
        if (d->id() != _device) {
            // Haven't found device
            continue;
        }

        found_device = true;

        // Verify codename
        auto codenames = d->codenames();
        auto it = std::find_if(codenames.begin(), codenames.end(),
                               [&](const std::string &codename) {
            return prop_product_device.find(codename) != std::string::npos
                    || prop_build_product.find(codename) != std::string::npos;
        });

        if (it == codenames.end()) {
            display_msg("Patched zip is for:");
            for (const std::string &codename : d->codenames()) {
                display_msg(fmt::format("- {}", codename));
            }
            display_msg(fmt::format("This device is '{}'", prop_product_device));

            return ProceedState::Fail;
        }

        // Copy boot partition block devices to the chroot
        auto devs = d->bootBlockDevs();
        if (devs.empty()) {
            display_msg("Could not determine the boot block device");
            return ProceedState::Fail;
        }

        _boot_block_dev = devs[0];
        LOGD("Boot block device: {}", _boot_block_dev);

        // Recovery block devices
        auto recovery_devs = d->recoveryBlockDevs();
        if (recovery_devs.empty()) {
            display_msg("Could not determine the recovery block device");
            return ProceedState::Fail;
        }

        _recovery_block_dev = recovery_devs[0];
        LOGD("Recovery block device: {}", _recovery_block_dev);

        // Copy any other required block devices to the chroot
        auto extra_devs = d->extraBlockDevs();

        devs.insert(devs.end(), recovery_devs.begin(), recovery_devs.end());
        devs.insert(devs.end(), extra_devs.begin(), extra_devs.end());

        for (auto const &dev : devs) {
            std::string dev_path(_chroot);
            dev_path += "/";
            dev_path += dev;

            if (!util::mkdir_parent(dev_path, 0755)) {
                LOGE("Failed to create parent directory of {}", dev_path);
            }

            // Follow symlinks just in case the symlink source isn't in the list
            if (!util::copy_file(dev, dev_path, util::MB_COPY_ATTRIBUTES
                                              | util::MB_COPY_XATTRS
                                              | util::MB_COPY_FOLLOW_SYMLINKS)) {
                LOGE("Failed to copy {}. Continuing anyway", dev);
            }

            LOGD("Copied {} to the chroot", dev);
        }

        break;
    }

    if (!found_device) {
        display_msg("Invalid device ID: " + _device);
        return ProceedState::Fail;
    }

    return on_checked_device();
}

Installer::ProceedState Installer::install_stage_get_install_type()
{
    LOGD("[Installer] Retrieve install type stage");

    std::vector<std::shared_ptr<Rom>> roms;
    mb_roms_add_builtin(&roms);

    std::string install_type = get_install_type();

    if (install_type == CANCELLED) {
        display_msg("Cancelled installation");
        return ProceedState::Cancel;
    }

    _rom = mb_find_rom_by_id(&roms, install_type);
    if (!_rom) {
        display_msg(fmt::format("Unknown ROM ID: {}", install_type));
        return ProceedState::Fail;
    }

    // Use raw paths if needed
    struct stat sb;
    if (stat("/raw-system", &sb) == 0) {
        // Old style: /system -> /raw-system, etc.
        _rom->system_path.insert(1, "raw-");
        _rom->cache_path.insert(1, "raw-");
        _rom->data_path.insert(1, "raw-");
    } else if (stat("/raw", &sb) == 0) {
        // New style: /system -> /raw/system, etc.
        _rom->system_path.insert(0, "/raw");
        _rom->cache_path.insert(0, "/raw");
        _rom->data_path.insert(0, "/raw");
    }

    display_msg("ROM ID: " + _rom->id);
    display_msg("- /system: " + _rom->system_path);
    display_msg("- /cache: " + _rom->cache_path);
    display_msg("- /data: " + _rom->data_path);

    return ProceedState::Continue;
}

Installer::ProceedState Installer::install_stage_set_up_chroot()
{
    LOGD("[Installer] Chroot set up stage");

    // Calculate SHA1 hash of the boot partition
    if (!util::sha1_hash(_boot_block_dev, _boot_hash)) {
        display_msg("Failed to compute sha1sum of boot partition");
        return ProceedState::Fail;
    }

    std::string digest = util::hex_string(_boot_hash, SHA_DIGEST_SIZE);
    LOGD("Boot partition SHA1sum: {}", digest);

    // Save a copy of the boot image that we'll restore if the installation fails
    if (!util::copy_contents(_boot_block_dev, _temp + "/boot.orig")) {
        display_msg("Failed to backup boot partition");
        return ProceedState::Fail;
    }

    // Extract busybox's unzip tool with support for zip file data descriptors
    if (!set_up_unzip()) {
        display_msg("Failed to extract unzip tool");
        return ProceedState::Fail;
    }

    // Wrap busybox to disable some applets
    if (!set_up_busybox_wrapper()) {
        display_msg("Failed to extract busybox wrapper");
        return ProceedState::Fail;
    }

    // Copy ourself for the real update-binary to use
    util::copy_file(mb_self_get_path(), in_chroot(HELPER_TOOL),
                    util::MB_COPY_ATTRIBUTES | util::MB_COPY_XATTRS);
    chmod(in_chroot(HELPER_TOOL).c_str(), 0555);

    // Copy /default.prop
    util::copy_file("/default.prop", in_chroot("/default.prop"),
                    util::MB_COPY_ATTRIBUTES | util::MB_COPY_XATTRS);

    // Copy file_contexts
    util::copy_file("/file_contexts", in_chroot("/file_contexts"),
                    util::MB_COPY_ATTRIBUTES | util::MB_COPY_XATTRS);


    return on_set_up_chroot();
}

Installer::ProceedState Installer::install_stage_mount_filesystems()
{
    LOGD("[Installer] Filesystem mounting stage");

    // Mount target filesystems
    if (!util::bind_mount(_rom->cache_path, 0771,
                          in_chroot("/cache"), 0771)) {
        display_msg(fmt::format("Failed to bind mount {} to {}",
                                _rom->cache_path, in_chroot("/cache")));
        return ProceedState::Fail;
    }

    if (!util::bind_mount(_rom->data_path, 0771,
                          in_chroot("/data"), 0771)) {
        display_msg(fmt::format("Failed to bind mount {} to {}",
                                _rom->data_path, in_chroot("/data")));
        return ProceedState::Fail;
    }

    // Create a temporary image if the zip file has a system.transfer.list file

    if (!_has_block_image && _rom->id != "primary") {
        if (!util::bind_mount(_rom->system_path, 0771,
                              in_chroot("/system"), 0771)) {
            display_msg(fmt::format("Failed to bind mount {} to {}",
                                    _rom->system_path, in_chroot("/system")));
            return ProceedState::Fail;
        }
    } else {
        display_msg("Copying system to temporary image");

        // Create temporary image in /data
        if (!create_temporary_image(TEMP_SYSTEM_IMAGE)) {
            display_msg(fmt::format("Failed to create temporary image {}",
                                    TEMP_SYSTEM_IMAGE));
            return ProceedState::Fail;
        }

        // Copy current /system files to the image
        if (!system_image_copy(_rom->system_path, TEMP_SYSTEM_IMAGE, false)) {
            display_msg(fmt::format("Failed to copy {} to {}",
                                    _rom->system_path, TEMP_SYSTEM_IMAGE));
            return ProceedState::Fail;
        }

        // Install to the image
        util::create_empty_file(in_chroot("/mb/system.img"));
        if (log_mount(TEMP_SYSTEM_IMAGE.c_str(),
                      in_chroot("/mb/system.img").c_str(),
                      "", MS_BIND, "") < 0) {
            return ProceedState::Fail;
        }
    }


    // Bind-mount zip file
    util::create_empty_file(in_chroot("/mb/install.zip"));
    if (log_mount(_zip_file.c_str(), in_chroot("/mb/install.zip").c_str(),
                  "", MS_BIND, "") < 0) {
        return ProceedState::Fail;
    }


    return on_mounted_filesystems();
}

Installer::ProceedState Installer::install_stage_installation()
{
    LOGD("[Installer] Installation stage");

    ProceedState hook_ret = on_pre_install();
    if (hook_ret != ProceedState::Continue) return hook_ret;


    // Run real update-binary
    display_msg("Running real update-binary");
    display_msg("Here we go!");

    bool updater_ret = run_real_updater();
    if (!updater_ret) {
        display_msg("Failed to run real update-binary");
    }

#if DEBUG_SHELL
    {
        util::run_command_chroot(_chroot, { "/sbin/sh", "-i" });
    }
#endif


    hook_ret = on_post_install(updater_ret);
    if (hook_ret != ProceedState::Continue) return hook_ret;

    return updater_ret ? ProceedState::Continue : ProceedState::Fail;
}

Installer::ProceedState Installer::install_stage_unmount_filesystems()
{
    LOGD("[Installer] Filesystem unmounting stage");

    // Umount filesystems from inside the chroot
    util::run_command_chroot(_chroot, { HELPER_TOOL, "unmount", "/system" });
    util::run_command_chroot(_chroot, { HELPER_TOOL, "unmount", "/cache" });
    util::run_command_chroot(_chroot, { HELPER_TOOL, "unmount", "/data" });

    if (_has_block_image || _rom->id == "primary") {
        display_msg("Copying temporary image to system");

        // Format system directory
        if (!wipe_directory(_rom->system_path, true)) {
            display_msg(fmt::format("Failed to wipe {}", _rom->system_path));
            return ProceedState::Fail;
        }

        // Copy image back to system directory
        if (!system_image_copy(_rom->system_path, TEMP_SYSTEM_IMAGE, true)) {
            display_msg(fmt::format("Failed to copy {} to {}",
                                    TEMP_SYSTEM_IMAGE, _rom->system_path));
            return ProceedState::Fail;
        }
    }


    return on_unmounted_filesystems();
}

Installer::ProceedState Installer::install_stage_finish()
{
    LOGD("[Installer] Finalization stage");

    // Calculate SHA1 hash of the boot partition after installation
    unsigned char new_hash[SHA_DIGEST_SIZE];
    if (!util::sha1_hash(_boot_block_dev, new_hash)) {
        display_msg("Failed to compute sha1sum of boot partition");
        return ProceedState::Fail;
    }

    std::string old_digest = util::hex_string(_boot_hash, SHA_DIGEST_SIZE);
    std::string new_digest = util::hex_string(new_hash, SHA_DIGEST_SIZE);
    LOGD("Old boot partition SHA1sum: {}", old_digest);
    LOGD("New boot partition SHA1sum: {}", new_digest);

    // Set kernel if it was changed
    if (memcmp(_boot_hash, new_hash, SHA_DIGEST_SIZE) != 0) {
        display_msg("Boot partition was modified. Setting kernel");

        mbp::BootImage bi;
        if (!bi.load(_boot_block_dev)) {
            display_msg("Failed to load boot partition image");
            return ProceedState::Fail;
        }

        std::string cmdline = bi.kernelCmdline();
        cmdline += " romid=";
        cmdline += _rom->id;
        bi.setKernelCmdline(std::move(cmdline));

        auto bootimg = bi.create();
        bool was_loki = bi.isLoki();
        std::string temp_boot_img(_temp);
        temp_boot_img += "/boot.img";
        std::string temp_boot_lok(_temp);
        temp_boot_lok += "/boot.lok";

        // Backup kernel

        if (!util::file_write_data(temp_boot_img,
                                   reinterpret_cast<char *>(bootimg.data()),
                                   bootimg.size())) {
            LOGE("Failed to write {}: {}", temp_boot_img, strerror(errno));
            display_msg(fmt::format("Failed to write {}", temp_boot_img));
            return ProceedState::Fail;
        }

        // Reloki if needed
        if (was_loki) {
            if (!loki_patch_file(temp_boot_img, temp_boot_lok)) {
                display_msg("Failed to run loki");
                return ProceedState::Fail;
            }

            display_msg("Successfully loki'd boot image");

            unlink(temp_boot_img.c_str());
            rename(temp_boot_lok.c_str(), temp_boot_img.c_str());
        }

        // Write to multiboot directory and boot partition

        std::string path("/data/media/0/MultiBoot/");
        path += _rom->id;
        path += "/boot.img";
        if (!util::mkdir_parent(path, 0775)) {
            display_msg(fmt::format("Failed to create {}", path));
            return ProceedState::Fail;
        }

        int fd_source = open(temp_boot_img.c_str(), O_RDONLY);
        if (fd_source < 0) {
            LOGE("Failed to open {}: {}", temp_boot_img, strerror(errno));
            return ProceedState::Fail;
        }

        auto close_fd_source = util::finally([&] { close(fd_source); });

        int fd_boot = open(_boot_block_dev.c_str(), O_WRONLY);
        if (fd_boot < 0) {
            LOGE("Failed to open {}: {}", _boot_block_dev, strerror(errno));
            return ProceedState::Fail;
        }

        auto close_fd_boot = util::finally([&] { close(fd_boot); });

        int fd_backup = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0775);
        if (fd_backup < 0) {
            LOGE("Failed to open {}: {}", path, strerror(errno));
            return ProceedState::Fail;
        }

        auto close_fd_backup = util::finally([&] { close(fd_backup); });

        if (!util::copy_data_fd(fd_source, fd_boot)) {
            LOGE("Failed to write {}: {}", _boot_block_dev, strerror(errno));
            return ProceedState::Fail;
        }

        lseek(fd_source, 0, SEEK_SET);

        if (!util::copy_data_fd(fd_source, fd_backup)) {
            LOGE("Failed to write {}: {}", path, strerror(errno));
            return ProceedState::Fail;
        }

        if (fchmod(fd_backup, 0775) < 0) {
            // Non-fatal
            LOGE("{}: Failed to chmod: {}", path, strerror(errno));
        }

        if (!util::chown(path, "media_rw", "media_rw", 0)) {
            // Non-fatal
            LOGE("{}: Failed to chown: {}", path, strerror(errno));
        }
    }


    return on_finished();
}

void Installer::install_stage_cleanup(Installer::ProceedState ret)
{
    LOGD("[Installer] Cleanup stage");

    if (ret == ProceedState::Fail) {
        display_msg("Failed to flash zip file.");
    }

    display_msg("Destroying chroot environment");

    remove(TEMP_SYSTEM_IMAGE.c_str());

    if (ret == ProceedState::Fail && !_boot_block_dev.empty()
            && !util::copy_contents(_temp + "/boot.orig", _boot_block_dev)) {
        LOGE("Failed to restore boot partition: {}", strerror(errno));
        display_msg("Failed to restore boot partition");
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
    ProceedState ret;

    auto when_finished = util::finally([&] {
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