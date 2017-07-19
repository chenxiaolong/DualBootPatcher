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

#pragma once

#include "mbdevice/device.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MB_DEVICE_MISSING_ID                        (1ull << 0)
#define MB_DEVICE_MISSING_CODENAMES                 (1ull << 1)
#define MB_DEVICE_MISSING_NAME                      (1ull << 2)
#define MB_DEVICE_MISSING_ARCHITECTURE              (1ull << 3)
#define MB_DEVICE_MISSING_SYSTEM_BLOCK_DEVS         (1ull << 4)
#define MB_DEVICE_MISSING_CACHE_BLOCK_DEVS          (1ull << 5)
#define MB_DEVICE_MISSING_DATA_BLOCK_DEVS           (1ull << 6)
#define MB_DEVICE_MISSING_BOOT_BLOCK_DEVS           (1ull << 7)
#define MB_DEVICE_MISSING_RECOVERY_BLOCK_DEVS       (1ull << 8)
#define MB_DEVICE_MISSING_BOOT_UI_THEME             (1ull << 9)
#define MB_DEVICE_MISSING_BOOT_UI_GRAPHICS_BACKENDS (1ull << 10)

MB_EXPORT uint64_t mb_device_validate(const struct Device *device);

#ifdef __cplusplus
}
#endif
