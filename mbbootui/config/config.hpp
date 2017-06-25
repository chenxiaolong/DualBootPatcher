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

#include <sys/types.h>

#ifdef __cplusplus
#include <stdint.h>

extern "C" {
#endif

enum tw_flags
{
    // TWRP: RECOVERY_TOUCHSCREEN_SWAP_XY
    TW_FLAG_TOUCHSCREEN_SWAP_XY             = 0x1ull,
    // TWRP: RECOVERY_TOUCHSCREEN_FLIP_X
    TW_FLAG_TOUCHSCREEN_FLIP_X              = 0x2ull,
    // TWRP: RECOVERY_TOUCHSCREEN_FLIP_Y
    TW_FLAG_TOUCHSCREEN_FLIP_Y              = 0x4ull,
    // TWRP: RECOVERY_GRAPHICS_FORCE_USE_LINELENGTH
    TW_FLAG_GRAPHICS_FORCE_USE_LINELENGTH   = 0x8ull,
    // TWRP: TW_SCREEN_BLANK_ON_BOOT
    TW_FLAG_SCREEN_BLANK_ON_BOOT            = 0x10ull,
    // TWRP: BOARD_HAS_FLIPPED_SCREEN
    TW_FLAG_BOARD_HAS_FLIPPED_SCREEN        = 0x20ull,
    // TWRP: TW_IGNORE_MAJOR_AXIS_0
    TW_FLAG_IGNORE_MAJOR_AXIS_0             = 0x40ull,
    // TWRP: TW_IGNORE_MT_POSITION_0
    TW_FLAG_IGNORE_MT_POSITION_0            = 0x80ull,
    // TWRP: TW_IGNORE_ABS_MT_TRACKING_ID
    TW_FLAG_IGNORE_ABS_MT_TRACKING_ID       = 0x100ull,
    // TWRP: TW_NEW_ION_HEAP
    TW_FLAG_NEW_ION_HEAP                    = 0x200ull,
    // TWRP: TW_NO_SCREEN_BLANK
    TW_FLAG_NO_SCREEN_BLANK                 = 0x400ull,
    // TWRP: TW_NO_SCREEN_TIMEOUT
    TW_FLAG_NO_SCREEN_TIMEOUT               = 0x800ull,
    // TWRP: TW_ROUND_SCREEN
    TW_FLAG_ROUND_SCREEN                    = 0x1000ull,
    // TWRP: TW_NO_CPU_TEMP
    TW_FLAG_NO_CPU_TEMP                     = 0x2000ull,
    // TWRP: QCOM_RTC_FIX
    TW_FLAG_QCOM_RTC_FIX                    = 0x4000ull,
    TW_FLAG_HAS_DOWNLOAD_MODE               = 0x8000ull,
    TW_FLAG_PREFER_LCD_BACKLIGHT            = 0x10000ull,
};

enum tw_pixel_formats
{
    TW_PXFMT_DEFAULT,
    TW_PXFMT_ABGR_8888,
    TW_PXFMT_RGBX_8888,
    TW_PXFMT_BGRA_8888,
    TW_PXFMT_RGBA_8888
};

enum tw_force_pixel_formats
{
    TW_FORCE_PXFMT_NONE,
    TW_FORCE_PXFMT_RGB_565
};

extern uint64_t tw_flags;

// TWRP: TARGET_RECOVERY_PIXEL_FORMAT
extern int tw_pixel_format;
// TWRP: TARGET_RECOVERY_FORCE_PIXEL_FORMAT
extern int tw_force_pixel_format;
// TWRP: OVERSCAN_PERCENT
extern int tw_overscan_percent;

// TWRP: TW_INPUT_BLACKLIST
extern const char *tw_input_blacklist;
// TWRP: TW_WHITELIST_INPUT
extern const char *tw_whitelist_input;

// TWRP: TW_X_OFFSET
extern int tw_default_x_offset;
// TWRP: TW_Y_OFFSET
extern int tw_default_y_offset;

// TWRP: TW_BRIGHTNESS_PATH
extern const char *tw_brightness_path;
// TWRP: TW_SECONDARY_BRIGHTNESS_PATH
extern const char *tw_secondary_brightness_path;
// TWRP: TW_MAX_BRIGHTNESS
extern int tw_max_brightness;
// TWRP: TW_DEFAULT_BRIGHTNESS
extern int tw_default_brightness;
// TWRP: TW_CUSTOM_BATTERY_PATH
extern const char *tw_custom_battery_path;
// TWRP: TW_CUSTOM_CPU_TEMP_PATH
extern const char *tw_custom_cpu_temp_path;

extern const char *tw_resource_path;
extern const char *tw_settings_path;
extern const char *tw_screenshots_path;
extern const char *tw_theme_zip_path;

// TODO: Make TW_USE_KEY_CODE_TOUCH_SYNC an option

// Other variables
extern int tw_android_sdk_version;

// Backends
extern const char * const *tw_graphics_backends;
extern size_t tw_graphics_backends_length;

// Device
struct Device;
extern struct Device *tw_device;

#ifdef __cplusplus
}
#endif
