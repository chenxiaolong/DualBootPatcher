/*
 * Copyright (C) 2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbdevice/validate.h"

#include "mbdevice/internal/structs.h"

// Works for any NULL-terminated thing
#define NULL_OR_EMPTY(x)                (!(x) || !*(x))

uint64_t mb_device_validate(const struct Device *device)
{
    uint64_t flags = 0;

    if (NULL_OR_EMPTY(device->id)) {
        flags |= MB_DEVICE_MISSING_ID;
    }

    if (NULL_OR_EMPTY(device->codenames)) {
        flags |= MB_DEVICE_MISSING_CODENAMES;
    }

    if (NULL_OR_EMPTY(device->name)) {
        flags |= MB_DEVICE_MISSING_NAME;
    }

    if (NULL_OR_EMPTY(device->architecture)) {
        flags |= MB_DEVICE_MISSING_ARCHITECTURE;
    }

    if (NULL_OR_EMPTY(device->system_devs)) {
        flags |= MB_DEVICE_MISSING_SYSTEM_BLOCK_DEVS;
    }

    if (NULL_OR_EMPTY(device->cache_devs)) {
        flags |= MB_DEVICE_MISSING_CACHE_BLOCK_DEVS;
    }

    if (NULL_OR_EMPTY(device->data_devs)) {
        flags |= MB_DEVICE_MISSING_DATA_BLOCK_DEVS;
    }

    if (NULL_OR_EMPTY(device->boot_devs)) {
        flags |= MB_DEVICE_MISSING_BOOT_BLOCK_DEVS;
    }

    //if (NULL_OR_EMPTY(device->recovery_devs)) {
    //    flags |= MB_DEVICE_MISSING_RECOVERY_BLOCK_DEVS;
    //}

    if (device->tw_options.supported) {
        if (NULL_OR_EMPTY(device->tw_options.theme)) {
            flags |= MB_DEVICE_MISSING_BOOT_UI_THEME;
        }

        if (NULL_OR_EMPTY(device->tw_options.graphics_backends)) {
            flags |= MB_DEVICE_MISSING_BOOT_UI_GRAPHICS_BACKENDS;
        }
    }

    return flags;
}
