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
#include "util/path.h"
#include "util/properties.h"

#define MULTIBOOT_DIR "/data/media/0/MultiBoot"

static int choose_or_set_rom(const char *id, const char *boot_blockdev, int choose)
{
    char *bootimg_path = NULL;

    struct roms roms;
    mb_roms_init(&roms);
    mb_roms_get_installed(&roms);

    struct rom *r = mb_find_rom_by_id(&roms, id);
    if (!r) {
        LOGE("Invalid ROM ID: %s", id);
        goto error;
    }

    bootimg_path = mb_path_build(MULTIBOOT_DIR, id, "boot.img", NULL);
    if (!bootimg_path) {
        LOGE("Out of memory");
        goto error;
    }

    if (mb_mkdir_parent(bootimg_path, 0775) < 0) {
        LOGE("Failed to create parent directory of %s", bootimg_path);
        goto error;
    }

    if (mb_copy_contents(choose ? bootimg_path : boot_blockdev,
                         choose ? boot_blockdev : bootimg_path) < 0) {
        LOGE("Failed to write %s", choose ? boot_blockdev : bootimg_path);
        goto error;
    }

    if (mb_chown(MULTIBOOT_DIR, "media_rw", "media_rw", MB_CHOWN_RECURSIVE) < 0) {
        LOGE("Failed to chown %s", MULTIBOOT_DIR);
        goto error;
    }

    if (mb_chmod_recursive(MULTIBOOT_DIR, 0775) < 0) {
        LOGE("Failed to chmod %s", MULTIBOOT_DIR);
    }

    mb_roms_cleanup(&roms);
    free(bootimg_path);
    return 0;

error:
    mb_roms_cleanup(&roms);
    free(bootimg_path);
    return -1;
}

int mb_action_choose_rom(const char *id, const char *boot_blockdev)
{
    return choose_or_set_rom(id, boot_blockdev, 1);
}

int mb_action_set_kernel(const char *id, const char *boot_blockdev)
{
    return choose_or_set_rom(id, boot_blockdev, 0);
}

int mb_action_reboot(const char *reboot_arg)
{
    char value[MB_PROP_VALUE_MAX];

    if (!reboot_arg) {
        LOGE("NULL reboot argument!");
        return -1;
    }

    size_t len = snprintf(value, sizeof(value), "reboot,%s", reboot_arg);
    if (len >= sizeof(value)) {
        LOGE("Reboot argument %zu bytes too long", len - sizeof(value) + 1);
        return -1;
    }

    if (mb_set_property("sys.powerctl", value) < 0) {
        LOGE("Failed to set property");
        return -1;
    }

    // Obviously shouldn't return
    while (1) {
        pause();
    }

    return 0;
}