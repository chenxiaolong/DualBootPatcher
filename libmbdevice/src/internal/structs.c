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

#include "mbdevice/internal/structs.h"

#include <stdlib.h>
#include <string.h>

#include "mbdevice/internal/array.h"


static bool initialize_tw_options(struct TwOptions *tw_options)
{
    memset(tw_options, 0, sizeof(struct TwOptions));

    tw_options->pixel_format = TW_PIXEL_FORMAT_DEFAULT;
    tw_options->force_pixel_format = TW_FORCE_PIXEL_FORMAT_NONE;
    tw_options->max_brightness = -1;
    tw_options->default_brightness = -1;

    return true;
}

static void cleanup_tw_options(struct TwOptions *tw_options)
{
    if (tw_options) {
        free(tw_options->brightness_path);
        free(tw_options->secondary_brightness_path);
        free(tw_options->battery_path);
        free(tw_options->cpu_temp_path);
        free(tw_options->input_blacklist);
        free(tw_options->input_whitelist);
        string_array_free(tw_options->graphics_backends);
        free(tw_options->theme);
    }
}

bool initialize_device(struct Device *device)
{
    memset(device, 0, sizeof(struct Device));

    return initialize_tw_options(&device->tw_options);
}

void cleanup_device(struct Device *device)
{
    if (device) {
        free(device->id);
        string_array_free(device->codenames);
        free(device->name);
        free(device->architecture);
        string_array_free(device->base_dirs);
        string_array_free(device->system_devs);
        string_array_free(device->cache_devs);
        string_array_free(device->data_devs);
        string_array_free(device->boot_devs);
        string_array_free(device->recovery_devs);
        string_array_free(device->extra_devs);

        cleanup_tw_options(&device->tw_options);
    }
}
