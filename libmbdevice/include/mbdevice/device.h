/*
 * Copyright (C) 2014-2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include <stdbool.h>
#include <stdint.h>

#include "mbcommon/common.h"
#include "mbdevice/flags.h"


#define MB_DEVICE_OK                    0
#define MB_DEVICE_ERROR_ERRNO           (-1)
#define MB_DEVICE_ERROR_INVALID_VALUE   (-2)


struct Device;

#ifdef __cplusplus
extern "C" {
#endif

MB_EXPORT struct Device * mb_device_new();

MB_EXPORT void mb_device_free(struct Device *device);

#define GETTER(TYPE, NAME) \
    MB_EXPORT TYPE mb_device_ ## NAME (const struct Device *device)
#define SETTER(TYPE, NAME) \
    MB_EXPORT int mb_device_set_ ## NAME (struct Device *device, TYPE value)

GETTER(const char *, id);
SETTER(const char *, id);

GETTER(char const * const *, codenames);
SETTER(char const * const *, codenames);

GETTER(const char *, name);
SETTER(const char *, name);

GETTER(const char *, architecture);
SETTER(const char *, architecture);

GETTER(uint64_t, flags);
SETTER(uint64_t, flags);

GETTER(char const * const *, block_dev_base_dirs);
SETTER(char const * const *, block_dev_base_dirs);

GETTER(char const * const *, system_block_devs);
SETTER(char const * const *, system_block_devs);

GETTER(char const * const *, cache_block_devs);
SETTER(char const * const *, cache_block_devs);

GETTER(char const * const *, data_block_devs);
SETTER(char const * const *, data_block_devs);

GETTER(char const * const *, boot_block_devs);
SETTER(char const * const *, boot_block_devs);

GETTER(char const * const *, recovery_block_devs);
SETTER(char const * const *, recovery_block_devs);

GETTER(char const * const *, extra_block_devs);
SETTER(char const * const *, extra_block_devs);

/* Boot UI */

GETTER(bool, tw_supported);
SETTER(bool, tw_supported);

GETTER(uint64_t, tw_flags);
SETTER(uint64_t, tw_flags);

GETTER(enum TwPixelFormat, tw_pixel_format);
SETTER(enum TwPixelFormat, tw_pixel_format);

GETTER(enum TwForcePixelFormat, tw_force_pixel_format);
SETTER(enum TwForcePixelFormat, tw_force_pixel_format);

GETTER(int, tw_overscan_percent);
SETTER(int, tw_overscan_percent);

GETTER(int, tw_default_x_offset);
SETTER(int, tw_default_x_offset);

GETTER(int, tw_default_y_offset);
SETTER(int, tw_default_y_offset);

GETTER(const char *, tw_brightness_path);
SETTER(const char *, tw_brightness_path);

GETTER(const char *, tw_secondary_brightness_path);
SETTER(const char *, tw_secondary_brightness_path);

GETTER(int, tw_max_brightness);
SETTER(int, tw_max_brightness);

GETTER(int, tw_default_brightness);
SETTER(int, tw_default_brightness);

GETTER(const char *, tw_battery_path);
SETTER(const char *, tw_battery_path);

GETTER(const char *, tw_cpu_temp_path);
SETTER(const char *, tw_cpu_temp_path);

GETTER(const char *, tw_input_blacklist);
SETTER(const char *, tw_input_blacklist);

GETTER(const char *, tw_input_whitelist);
SETTER(const char *, tw_input_whitelist);

GETTER(char const * const *, tw_graphics_backends);
SETTER(char const * const *, tw_graphics_backends);

GETTER(const char *, tw_theme);
SETTER(const char *, tw_theme);


MB_EXPORT bool mb_device_equals(struct Device *a, struct Device *b);

#undef GETTER
#undef SETTER

#ifdef __cplusplus
}
#endif
