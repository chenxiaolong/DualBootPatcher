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

#include "actions.h"

#include <cstdio>
#include <cstdlib>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include "roms.h"
#include "util/chmod.h"
#include "util/chown.h"
#include "util/copy.h"
#include "util/directory.h"
#include "util/finally.h"
#include "util/logging.h"
#include "util/properties.h"
#include "util/string.h"

#define MULTIBOOT_DIR "/data/media/0/MultiBoot"

namespace mb
{

static std::string find_block_dev(const std::vector<std::string> &search_dirs,
                                  const std::string &partition)
{
    struct stat sb;

    if (util::starts_with(partition, "mmcblk")) {
        std::string path("/dev/block/");
        path += partition;

        if (stat(path.c_str(), &sb) == 0) {
            return path;
        }
    }

    for (const std::string &base_dir : search_dirs) {
        std::string block_dev(base_dir);
        block_dev += "/";
        block_dev += partition;

        if (stat(block_dev.c_str(), &sb) == 0) {
            return block_dev;
        }
    }

    return std::string();
}

static bool flash_extra_images(const std::string &multiboot_dir,
                               const std::vector<std::string> &block_dev_dirs)
{
    DIR *dir;
    dirent *ent;

    dir = opendir(multiboot_dir.c_str());
    if (!dir) {
        return false;
    }

    auto close_directory = util::finally([&]{
        closedir(dir);
    });

    while ((ent = readdir(dir))) {
        std::string name(ent->d_name);
        if (name == ".img" || !util::ends_with(name, ".img")) {
            // Skip non-images
            continue;
        }
        if (name == "boot.img") {
            // Skip boot images, which are handled separately
            continue;
        }

        std::string partition = name.substr(0, name.size() - 4);
        std::string path(multiboot_dir);
        path += "/";
        path += name;

        // Blacklist non-modem partition
        if (partition != "mdm" && partition != "modem") {
            LOGW("Partition {} is not whitelisted for flashing", partition);
            continue;
        }

        std::string block_dev = find_block_dev(block_dev_dirs, partition);
        if (block_dev.empty()) {
            LOGW("Couldn't find block device for partition {}", partition);
            continue;
        }

        LOGD("Flashing extra image");
        LOGD("- Source: {}", path);
        LOGD("- Target: {}", block_dev);

        if (!util::copy_contents(path, block_dev)) {
            LOGE("Failed to write {}", block_dev);
            return false;
        }
    }

    return true;
}

static bool choose_or_set_rom(const std::string &id,
                              const std::string &boot_blockdev,
                              const std::vector<std::string> &blockdev_base_dirs,
                              bool choose)
{
    // Path for all of the images
    std::string multiboot_path(MULTIBOOT_DIR);
    multiboot_path += "/";
    multiboot_path += id;

    std::string bootimg_path(multiboot_path);
    bootimg_path += "/boot.img";

    // Verify ROM ID
    std::vector<std::shared_ptr<Rom>> roms;
    mb_roms_add_installed(&roms);

    auto r = mb_find_rom_by_id(&roms, id);
    if (!r) {
        LOGE("Invalid ROM ID: {}", id);
        return false;
    }

    if (!util::mkdir_recursive(multiboot_path, 0775)) {
        LOGE("Failed to create directory {}", multiboot_path);
        return false;
    }

    // Flash or set kernel
    if (!util::copy_contents(choose ? bootimg_path : boot_blockdev,
                             choose ? boot_blockdev : bootimg_path)) {
        LOGE("Failed to write {}", choose ? boot_blockdev : bootimg_path);
        return false;
    }

    // Fix permissions
    if (!util::chown(MULTIBOOT_DIR, "media_rw", "media_rw",
                     util::MB_CHOWN_RECURSIVE)) {
        LOGE("Failed to chown {}", MULTIBOOT_DIR);
        return false;
    }

    if (!util::chmod_recursive(MULTIBOOT_DIR, 0775)) {
        LOGE("Failed to chmod {}", MULTIBOOT_DIR);
        return false;
    }

    // If there are additional image files, flash those too
    if (choose) {
        if (!flash_extra_images(multiboot_path, blockdev_base_dirs)) {
            LOGE("Failed to flash extra images");
            return false;
        }
    }

    return true;
}

bool action_choose_rom(const std::string &id, const std::string &boot_blockdev,
                       const std::vector<std::string> &blockdev_base_dirs)
{
    return choose_or_set_rom(id, boot_blockdev, blockdev_base_dirs, true);
}

bool action_set_kernel(const std::string &id, const std::string &boot_blockdev)
{
    return choose_or_set_rom(id, boot_blockdev, {}, false);
}

bool action_reboot(const std::string &reboot_arg)
{
    std::string value("reboot,");
    value.append(reboot_arg);

    if (value.size() >= MB_PROP_VALUE_MAX - 1) {
        LOGE("Reboot argument {:d} bytes too long",
             value.size() + 1 - MB_PROP_VALUE_MAX);
        return false;
    }

    if (!util::set_property("sys.powerctl", value)) {
        LOGE("Failed to set property");
        return false;
    }

    // Obviously shouldn't return
    while (1) {
        pause();
    }

    return true;
}

}