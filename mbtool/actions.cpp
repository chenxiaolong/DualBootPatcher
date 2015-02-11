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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "roms.h"
#include "util/chmod.h"
#include "util/chown.h"
#include "util/copy.h"
#include "util/directory.h"
#include "util/logging.h"
#include "util/properties.h"

#define MULTIBOOT_DIR "/data/media/0/MultiBoot"

static bool choose_or_set_rom(const std::string &id,
                              const std::string &boot_blockdev, bool choose)
{
    std::string bootimg_path;

    std::vector<std::shared_ptr<Rom>> roms;
    mb_roms_add_installed(&roms);

    auto r = mb_find_rom_by_id(&roms, id);
    if (!r) {
        LOGE("Invalid ROM ID: %s", id);
        return false;
    }

    bootimg_path.append(MULTIBOOT_DIR).append("/")
                .append(id).append("/")
                .append("boot.img");

    if (!MB::mkdir_parent(bootimg_path, 0775)) {
        LOGE("Failed to create parent directory of %s", bootimg_path);
        return false;
    }

    if (!MB::copy_contents(choose ? bootimg_path : boot_blockdev,
                           choose ? boot_blockdev : bootimg_path)) {
        LOGE("Failed to write %s", choose ? boot_blockdev : bootimg_path);
        return false;
    }

    if (!MB::chown(MULTIBOOT_DIR, "media_rw", "media_rw", MB_CHOWN_RECURSIVE)) {
        LOGE("Failed to chown %s", MULTIBOOT_DIR);
        return false;
    }

    if (!MB::chmod_recursive(MULTIBOOT_DIR, 0775)) {
        LOGE("Failed to chmod %s", MULTIBOOT_DIR);
        return false;
    }

    return true;
}

bool action_choose_rom(const std::string &id, const std::string &boot_blockdev)
{
    return choose_or_set_rom(id, boot_blockdev, true);
}

bool action_set_kernel(const std::string &id, const std::string &boot_blockdev)
{
    return choose_or_set_rom(id, boot_blockdev, false);
}

bool action_reboot(const std::string &reboot_arg)
{
    std::string value("reboot,");
    value.append(reboot_arg);

    if (value.size() >= MB_PROP_VALUE_MAX - 1) {
        LOGE("Reboot argument %zu bytes too long",
             value.size() + 1 - MB_PROP_VALUE_MAX);
        return false;
    }

    if (!MB::set_property("sys.powerctl", value)) {
        LOGE("Failed to set property");
        return false;
    }

    // Obviously shouldn't return
    while (1) {
        pause();
    }

    return true;
}