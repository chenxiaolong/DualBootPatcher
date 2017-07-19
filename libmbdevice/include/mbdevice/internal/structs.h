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

#include "mbdevice/flags.h"

struct TwOptions
{
    bool supported;

    uint64_t flags;

    enum TwPixelFormat pixel_format;
    enum TwForcePixelFormat force_pixel_format;

    int overscan_percent;
    int default_x_offset;
    int default_y_offset;

    char *brightness_path;
    char *secondary_brightness_path;
    int max_brightness;
    int default_brightness;

    char *battery_path;
    char *cpu_temp_path;

    char *input_blacklist;
    char *input_whitelist;

    char **graphics_backends;

    char *theme;
};

struct Device
{
    char *id;
    char **codenames;
    char *name;
    char *architecture;
    uint64_t flags;

    char **base_dirs;
    char **system_devs;
    char **cache_devs;
    char **data_devs;
    char **boot_devs;
    char **recovery_devs;
    char **extra_devs;

    struct TwOptions tw_options;
};

#ifdef __cplusplus
extern "C" {
#endif

bool initialize_device(struct Device *device);
void cleanup_device(struct Device *device);

#ifdef __cplusplus
}
#endif
