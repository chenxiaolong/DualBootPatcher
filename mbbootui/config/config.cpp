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

#include "config/config.hpp"

extern "C" {

uint64_t tw_flags = 0;

int tw_pixel_format = TW_PXFMT_DEFAULT;
int tw_force_pixel_format = TW_FORCE_PXFMT_NONE;
int tw_overscan_percent = 0;

const char *tw_input_blacklist = nullptr;
const char *tw_whitelist_input = nullptr;

int tw_default_x_offset = 0;
int tw_default_y_offset = 0;

const char *tw_brightness_path = nullptr;
const char *tw_secondary_brightness_path = nullptr;
int tw_max_brightness = -1;
int tw_default_brightness = -1;
const char *tw_custom_battery_path = nullptr;
const char *tw_custom_cpu_temp_path = nullptr;

const char *tw_resource_path = "/bootui/theme";
const char *tw_settings_path = "/bootui/settings.bin";
const char *tw_screenshots_path = "/bootui/screenshots";
const char *tw_theme_zip_path = "/bootui/theme.zip";

int tw_android_sdk_version = 0;

const char * const *tw_graphics_backends = nullptr;
size_t tw_graphics_backends_length = 0;

struct Device *tw_device = nullptr;

}
