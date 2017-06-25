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

/*
 * NOTE: Keep these options synchronized with mbbootui/config/config.{c,h}pp
 */

#define ARCH_ARMEABI_V7A        "armeabi-v7a"
#define ARCH_ARM64_V8A          "arm64-v8a"
#define ARCH_X86                "x86"
#define ARCH_X86_64             "x86_64"

enum DeviceFlags
{
    FLAG_HAS_COMBINED_BOOT_AND_RECOVERY     = 1 << 0,
    FLAG_FSTAB_SKIP_SDCARD0                 = 1 << 1,
};

enum TwFlags
{
    FLAG_TW_TOUCHSCREEN_SWAP_XY             = 1 << 0,
    FLAG_TW_TOUCHSCREEN_FLIP_X              = 1 << 1,
    FLAG_TW_TOUCHSCREEN_FLIP_Y              = 1 << 2,
    FLAG_TW_GRAPHICS_FORCE_USE_LINELENGTH   = 1 << 3,
    FLAG_TW_SCREEN_BLANK_ON_BOOT            = 1 << 4,
    FLAG_TW_BOARD_HAS_FLIPPED_SCREEN        = 1 << 5,
    FLAG_TW_IGNORE_MAJOR_AXIS_0             = 1 << 6,
    FLAG_TW_IGNORE_MT_POSITION_0            = 1 << 7,
    FLAG_TW_IGNORE_ABS_MT_TRACKING_ID       = 1 << 8,
    FLAG_TW_NEW_ION_HEAP                    = 1 << 9,
    FLAG_TW_NO_SCREEN_BLANK                 = 1 << 10,
    FLAG_TW_NO_SCREEN_TIMEOUT               = 1 << 11,
    FLAG_TW_ROUND_SCREEN                    = 1 << 12,
    FLAG_TW_NO_CPU_TEMP                     = 1 << 13,
    FLAG_TW_QCOM_RTC_FIX                    = 1 << 14,
    FLAG_TW_HAS_DOWNLOAD_MODE               = 1 << 15,
    FLAG_TW_PREFER_LCD_BACKLIGHT            = 1 << 16,
};

enum TwPixelFormat
{
    TW_PIXEL_FORMAT_DEFAULT,
    TW_PIXEL_FORMAT_ABGR_8888,
    TW_PIXEL_FORMAT_RGBX_8888,
    TW_PIXEL_FORMAT_BGRA_8888,
    TW_PIXEL_FORMAT_RGBA_8888
};

enum TwForcePixelFormat
{
    TW_FORCE_PIXEL_FORMAT_NONE,
    TW_FORCE_PIXEL_FORMAT_RGB_565
};
