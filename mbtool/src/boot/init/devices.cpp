/*
 * Copyright (C) 2007 The Android Open Source Project
 * Copyright (C) 2015-2018 Andrew Gunnerson <andrewgunnerson@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "boot/init/devices.h"

#include <memory>

#include <cerrno>
#include <cstring>

#include <sys/sysmacros.h>
#include <unistd.h>

#include "mbcommon/string.h"

#include "mblog/logging.h"

#include "mbutil/cmdline.h"
#include "mbutil/directory.h"
#include "mbutil/path.h"

#define LOG_TAG "mbtool/boot/init/devices"

namespace android {
namespace init {

static std::string GetBootDevice()
{
    std::string result;

    if (auto cmdline = mb::util::kernel_cmdline()) {
        auto it = cmdline.value().find("androidboot.bootdevice");
        if (it != cmdline.value().end() && it->second) {
            it->second->swap(result);
        }
    }

    return result;
}

/* Given a path that may start with a PCI device, populate the supplied buffer
 * with the PCI domain/bus number and the peripheral ID and return 0.
 * If it doesn't start with a PCI device, or there is some error, return -1 */
static bool FindPciDevicePrefix(const std::string& path, std::string* result) {
    result->clear();

    if (!mb::starts_with(path, "/devices/pci")) return false;

    /* Beginning of the prefix is the initial "pci" after "/devices/" */
    std::string::size_type start = 9;

    /* End of the prefix is two path '/' later, capturing the domain/bus number
     * and the peripheral ID. Example: pci0000:00/0000:00:1f.2 */
    auto end = path.find('/', start);
    if (end == std::string::npos) return false;

    end = path.find('/', end + 1);
    if (end == std::string::npos) return false;

    auto length = end - start;
    if (length <= 4) {
        // The minimum string that will get to this check is 'pci/', which is malformed,
        // so return false
        return false;
    }

    *result = path.substr(start, length);
    return true;
}

/* Given a path that may start with a virtual block device, populate
 * the supplied buffer with the virtual block device ID and return 0.
 * If it doesn't start with a virtual block device, or there is some
 * error, return -1 */
static bool FindVbdDevicePrefix(const std::string& path, std::string* result) {
    result->clear();

    if (!mb::starts_with(path, "/devices/vbd-")) return false;

    /* Beginning of the prefix is the initial "vbd-" after "/devices/" */
    std::string::size_type start = 13;

    /* End of the prefix is one path '/' later, capturing the
       virtual block device ID. Example: 768 */
    auto end = path.find('/', start);
    if (end == std::string::npos) return false;

    auto length = end - start;
    if (length == 0) return false;

    *result = path.substr(start, length);
    return true;
}

// Given a path that may start with a platform device, find the parent platform device by finding a
// parent directory with a 'subsystem' symlink that points to the platform bus.
// If it doesn't start with a platform device, return false
bool DeviceHandler::FindPlatformDevice(std::string path, std::string* platform_device_path) const {
    platform_device_path->clear();

    // Uevents don't contain the mount point, so we need to add it here.
    path.insert(0, sysfs_mount_point_);

    std::string directory = mb::util::dir_name(path);

    while (directory != "/" && directory != ".") {
        if (auto p = mb::util::real_path(directory + "/subsystem");
                p && p.value() == sysfs_mount_point_ + "/bus/platform") {
            // We need to remove the mount point that we added above before returning.
            directory.erase(0, sysfs_mount_point_.size());
            *platform_device_path = directory;
            return true;
        }

        auto last_slash = path.rfind('/');
        if (last_slash == std::string::npos) return false;

        path.erase(last_slash);
        directory = mb::util::dir_name(path);
    }

    return false;
}

void DeviceHandler::MakeDevice(const std::string& path, bool block, int major, int minor) const {
    mode_t mode = 0600 | (block ? S_IFBLK : S_IFCHR);

    dev_t dev = static_cast<dev_t>(makedev(major, minor));
    if (mknod(path.c_str(), mode, dev) < 0) {
        LOGW("%s: Failed to create device: %s", path.c_str(), strerror(errno));
    }
}

// replaces any unacceptable characters with '_', the
// length of the resulting string is equal to the input string
void SanitizePartitionName(std::string* string) {
    const char* accept =
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "0123456789"
        "_-.";

    if (!string) return;

    std::string::size_type pos = 0;
    while ((pos = string->find_first_not_of(accept, pos)) != std::string::npos) {
        (*string)[pos] = '_';
    }
}

std::vector<std::string> DeviceHandler::GetBlockDeviceSymlinks(const Uevent& uevent) const {
    std::string device;
    std::string type;

    if (FindPlatformDevice(uevent.path, &device)) {
        // Skip /devices/platform or /devices/ if present
        static const std::string devices_platform_prefix = "/devices/platform/";
        static const std::string devices_prefix = "/devices/";

        if (mb::starts_with(device, devices_platform_prefix)) {
            device = device.substr(devices_platform_prefix.length());
        } else if (mb::starts_with(device, devices_prefix)) {
            device = device.substr(devices_prefix.length());
        }

        type = "platform";
    } else if (FindPciDevicePrefix(uevent.path, &device)) {
        type = "pci";
    } else if (FindVbdDevicePrefix(uevent.path, &device)) {
        type = "vbd";
    } else {
        return {};
    }

    std::vector<std::string> links;

    LOGV("found %s device %s", type.c_str(), device.c_str());

    std::vector<std::string> link_paths{
        "/dev/block/" + type + "/" + device
    };

    // Legacy /dev/block/bootdevice support
    if (!boot_device_) {
        boot_device_ = GetBootDevice();
    }
    if (!boot_device_->empty()
            && device.find(*boot_device_) != std::string::npos) {
        LOGE("Boot device is %s", device.c_str());
        link_paths.emplace_back("/dev/block/bootdevice");
    }

    for (auto const &link_path : link_paths) {
        if (!uevent.partition_name.empty()) {
            std::string partition_name_sanitized(uevent.partition_name);
            SanitizePartitionName(&partition_name_sanitized);
            if (partition_name_sanitized != uevent.partition_name) {
                LOGV("Linking partition '%s' as '%s'",
                     uevent.partition_name.c_str(), partition_name_sanitized.c_str());
            }
            links.emplace_back(link_path + "/by-name/" + partition_name_sanitized);
        }

        if (uevent.partition_num >= 0) {
            links.emplace_back(link_path + "/by-num/p" + std::to_string(uevent.partition_num));
        }

        auto last_slash = uevent.path.rfind('/');
        links.emplace_back(link_path + "/" + uevent.path.substr(last_slash + 1));
    }

    return links;
}

BlockDevMap DeviceHandler::GetBlockDeviceMap() const
{
    std::lock_guard<std::mutex> lock(block_dev_mappings_guard_);
    return block_dev_mappings_;
}

void DeviceHandler::HandleDevice(const std::string& action, const std::string& devpath, bool block,
                                 int major, int minor, const std::vector<std::string>& links) const {
    if (action == "add") {
        MakeDevice(devpath, block, major, minor);
        for (const auto& link : links) {
            if (auto r = mb::util::mkdir_parent(link, 0755); !r) {
                LOGE("Failed to create directory %s: %s",
                     mb::util::dir_name(link).c_str(),
                     r.error().message().c_str());
            }

            if (symlink(devpath.c_str(), link.c_str()) && errno != EEXIST) {
                LOGE("Failed to symlink %s to %s: %s",
                     devpath.c_str(), link.c_str(), strerror(errno));
            }
        }
    }

    if (action == "remove") {
        for (const auto& link : links) {
            if (auto p = mb::util::read_link(link); p && p.value() == devpath) {
                unlink(link.c_str());
            }
        }
        unlink(devpath.c_str());
    }
}

void DeviceHandler::HandleDeviceEvent(const Uevent& uevent) {
    // if it's not a /dev device, nothing to do
    if (uevent.major < 0 || uevent.minor < 0) return;

    std::string devpath;
    std::vector<std::string> links;
    bool block = false;

    if (uevent.subsystem == "block") {
        block = true;
        devpath = "/dev/block/" + mb::util::base_name(uevent.path);

        if (mb::starts_with(uevent.path, "/devices")) {
            links = GetBlockDeviceSymlinks(uevent);
        }
    } else if (mb::starts_with(uevent.subsystem, "usb")) {
        if (uevent.subsystem == "usb") {
            if (!uevent.device_name.empty()) {
                devpath = "/dev/" + uevent.device_name;
            } else {
                // This imitates the file system that would be created
                // if we were using devfs instead.
                // Minors are broken up into groups of 128, starting at "001"
                int bus_id = uevent.minor / 128 + 1;
                int device_id = uevent.minor % 128 + 1;
                devpath = mb::format("/dev/bus/usb/%03d/%03d", bus_id, device_id);
            }
        } else {
            // ignore other USB events
            return;
        }
    // Keep up to date with ueventd.rc
    } else if (uevent.subsystem == "adf") {
        devpath = "/dev/" + uevent.device_name;
    } else if (uevent.subsystem == "graphics") {
        devpath = "/dev/graphics/" + mb::util::base_name(uevent.path);
    } else if (uevent.subsystem == "drm") {
        devpath = "/dev/dri/" + mb::util::base_name(uevent.path);
    } else if (uevent.subsystem == "oncrpc") {
        devpath = "/dev/oncrpc/" + mb::util::base_name(uevent.path);
    } else if (uevent.subsystem == "adsp") {
        devpath = "/dev/adsp/" + mb::util::base_name(uevent.path);
    } else if (uevent.subsystem == "msm_camera") {
        devpath = "/dev/msm_camera/" + mb::util::base_name(uevent.path);
    } else if (uevent.subsystem == "input") {
        devpath = "/dev/input/" + mb::util::base_name(uevent.path);
    } else if (uevent.subsystem == "mtd") {
        devpath = "/dev/mtd/" + mb::util::base_name(uevent.path);
    } else if (uevent.subsystem == "sound") {
        devpath = "/dev/snd/" + mb::util::base_name(uevent.path);
    } else {
        devpath = "/dev/" + mb::util::base_name(uevent.path);
    }

    (void) mb::util::mkdir_parent(devpath, 0755);

    HandleDevice(uevent.action, devpath, block, uevent.major, uevent.minor, links);

    // Add/remove block device mapping
    if (uevent.subsystem == "block") {
        if (uevent.action == "add") {
            BlockDevInfo info;
            info.path = devpath;
            info.partition_num = uevent.partition_num;
            info.major = uevent.major;
            info.minor = uevent.minor;
            info.partition_name = uevent.partition_name;

            std::lock_guard<std::mutex> lock(block_dev_mappings_guard_);
            block_dev_mappings_.insert_or_assign(uevent.path, std::move(info));
        } else if (uevent.action == "remove") {
            std::lock_guard<std::mutex> lock(block_dev_mappings_guard_);
            block_dev_mappings_.erase(uevent.path);
        }
    }
}

DeviceHandler::DeviceHandler()
    : sysfs_mount_point_("/sys") {}

}  // namespace init
}  // namespace android
