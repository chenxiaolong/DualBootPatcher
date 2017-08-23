/*
 * Copyright (C) 2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#define MB_DEVICE_ARCH_ARMEABI_V7A                      "armeabi-v7a"
#define MB_DEVICE_ARCH_ARM64_V8A                        "arm64-v8a"
#define MB_DEVICE_ARCH_X86                              "x86"
#define MB_DEVICE_ARCH_X86_64                           "x86_64"

#define MB_DEVICE_FLAG_HAS_COMBINED_BOOT_AND_RECOVERY   (1u << 0)
#define MB_DEVICE_FLAG_FSTAB_SKIP_SDCARD0               (1u << 1)

#define MB_DEVICE_FLAG_MASK                             ((1u << 2) - 1)

#define MB_DEVICE_TW_FLAG_TOUCHSCREEN_SWAP_XY           (1u << 0)
#define MB_DEVICE_TW_FLAG_TOUCHSCREEN_FLIP_X            (1u << 1)
#define MB_DEVICE_TW_FLAG_TOUCHSCREEN_FLIP_Y            (1u << 2)
#define MB_DEVICE_TW_FLAG_GRAPHICS_FORCE_USE_LINELENGTH (1u << 3)
#define MB_DEVICE_TW_FLAG_SCREEN_BLANK_ON_BOOT          (1u << 4)
#define MB_DEVICE_TW_FLAG_BOARD_HAS_FLIPPED_SCREEN      (1u << 5)
#define MB_DEVICE_TW_FLAG_IGNORE_MAJOR_AXIS_0           (1u << 6)
#define MB_DEVICE_TW_FLAG_IGNORE_MT_POSITION_0          (1u << 7)
#define MB_DEVICE_TW_FLAG_IGNORE_ABS_MT_TRACKING_ID     (1u << 8)
#define MB_DEVICE_TW_FLAG_NEW_ION_HEAP                  (1u << 9)
#define MB_DEVICE_TW_FLAG_NO_SCREEN_BLANK               (1u << 10)
#define MB_DEVICE_TW_FLAG_NO_SCREEN_TIMEOUT             (1u << 11)
#define MB_DEVICE_TW_FLAG_ROUND_SCREEN                  (1u << 12)
#define MB_DEVICE_TW_FLAG_NO_CPU_TEMP                   (1u << 13)
#define MB_DEVICE_TW_FLAG_QCOM_RTC_FIX                  (1u << 14)
#define MB_DEVICE_TW_FLAG_HAS_DOWNLOAD_MODE             (1u << 15)
#define MB_DEVICE_TW_FLAG_PREFER_LCD_BACKLIGHT          (1u << 16)

#define MB_DEVICE_TW_FLAG_MASK                          ((1u << 17) - 1)

#define MB_DEVICE_TW_PXFMT_DEFAULT                      (0u)
#define MB_DEVICE_TW_PXFMT_ABGR_8888                    (1u)
#define MB_DEVICE_TW_PXFMT_RGBX_8888                    (2u)
#define MB_DEVICE_TW_PXFMT_BGRA_8888                    (3u)
#define MB_DEVICE_TW_PXFMT_RGBA_8888                    (4u)

#define MB_DEVICE_TW_FORCE_PXFMT_NONE                   (0u)
#define MB_DEVICE_TW_FORCE_PXFMT_RGB_565                (1u)

#define MB_DEVICE_V_FLAG_MISSING_ID                         (1u << 0)
#define MB_DEVICE_V_FLAG_MISSING_CODENAMES                  (1u << 1)
#define MB_DEVICE_V_FLAG_MISSING_NAME                       (1u << 2)
#define MB_DEVICE_V_FLAG_MISSING_ARCHITECTURE               (1u << 3)
#define MB_DEVICE_V_FLAG_MISSING_SYSTEM_BLOCK_DEVS          (1u << 4)
#define MB_DEVICE_V_FLAG_MISSING_CACHE_BLOCK_DEVS           (1u << 5)
#define MB_DEVICE_V_FLAG_MISSING_DATA_BLOCK_DEVS            (1u << 6)
#define MB_DEVICE_V_FLAG_MISSING_BOOT_BLOCK_DEVS            (1u << 7)
#define MB_DEVICE_V_FLAG_MISSING_RECOVERY_BLOCK_DEVS        (1u << 8)
#define MB_DEVICE_V_FLAG_MISSING_BOOT_UI_THEME              (1u << 9)
#define MB_DEVICE_V_FLAG_MISSING_BOOT_UI_GRAPHICS_BACKENDS  (1u << 10)
#define MB_DEVICE_V_FLAG_INVALID_ARCHITECTURE               (1u << 11)
#define MB_DEVICE_V_FLAG_INVALID_FLAGS                      (1u << 12)
#define MB_DEVICE_V_FLAG_INVALID_BOOT_UI_FLAGS              (1u << 13)
