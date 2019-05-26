/*
 * Copyright (C) 2014-2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbcommon/flags.h"

namespace mb::device
{

constexpr char ARCH_ARMEABI_V7A[]   = "armeabi-v7a";
constexpr char ARCH_ARM64_V8A[]     = "arm64-v8a";
constexpr char ARCH_X86[]           = "x86";
constexpr char ARCH_X86_64[]        = "x86_64";

enum class DeviceFlag : uint32_t
{
    HasCombinedBootAndRecovery  = 1 << 0,
    FstabSkipSdcard0            = 1 << 1,
};
MB_DECLARE_FLAGS(DeviceFlags, DeviceFlag)
MB_DECLARE_OPERATORS_FOR_FLAGS(DeviceFlags)

constexpr std::underlying_type_t<DeviceFlag> DEVICE_FLAG_MASK = (1 << 2) - 1;

enum class TwFlag : uint32_t
{
    TouchscreenSwapXY           = 1 << 0,
    TouchscreenFlipX            = 1 << 1,
    TouchscreenFlipY            = 1 << 2,
    GraphicsForceUseLineLength  = 1 << 3,
    ScreenBlankOnBoot           = 1 << 4,
    BoardHasFlippedScreen       = 1 << 5,
    IgnoreMajorAxis0            = 1 << 6,
    IgnoreMtPosition0           = 1 << 7,
    IgnoreAbsMtTrackingId       = 1 << 8,
    NewIonHeap                  = 1 << 9,
    NoScreenBlank               = 1 << 10,
    NoScreenTimeout             = 1 << 11,
    RoundScreen                 = 1 << 12,
    NoCpuTemp                   = 1 << 13,
    QcomRtcFix                  = 1 << 14,
    HasDownloadMode             = 1 << 15,
    PreferLcdBacklight          = 1 << 16,
};
MB_DECLARE_FLAGS(TwFlags, TwFlag)
MB_DECLARE_OPERATORS_FOR_FLAGS(TwFlags)

constexpr std::underlying_type_t<TwFlag> TW_FLAG_MASK = (1 << 17) - 1;

enum class TwPixelFormat : uint16_t
{
    Default = 0u,
    Abgr8888,
    Rgbx8888,
    Bgra8888,
    Rgba8888,
};

enum class TwForcePixelFormat : uint16_t
{
    None = 0u,
    Rgb565,
};

enum class ValidateFlag : uint32_t
{
    MissingId                       = 1u << 0,
    MissingCodenames                = 1u << 1,
    MissingName                     = 1u << 2,
    MissingArchitecture             = 1u << 3,
    MissingSystemBlockDevs          = 1u << 4,
    MissingCacheBlockDevs           = 1u << 5,
    MissingDataBlockDevs            = 1u << 6,
    MissingBootBlockDevs            = 1u << 7,
    MissingRecoveryBlockDevs        = 1u << 8,
    MissingBootUiTheme              = 1u << 9,
    MissingBootUiGraphicsBackends   = 1u << 10,
    InvalidArchitecture             = 1u << 11,
    InvalidFlags                    = 1u << 12,
    InvalidBootUiFlags              = 1u << 13,
};
MB_DECLARE_FLAGS(ValidateFlags, ValidateFlag)
MB_DECLARE_OPERATORS_FOR_FLAGS(ValidateFlags)

}
