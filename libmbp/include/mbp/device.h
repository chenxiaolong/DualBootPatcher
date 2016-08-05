/*
 * Copyright (C) 2014-2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
 *
 * This file is part of MultiBootPatcher
 *
 * MultiBootPatcher is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MultiBootPatcher is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MultiBootPatcher.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "mbcommon/common.h"

#define ARCH_ARMEABI_V7A        "armeabi-v7a"
#define ARCH_ARM64_V8A          "arm64-v8a"
#define ARCH_X86                "x86"
#define ARCH_X86_64             "x86_64"

#define TW_THEME_PORTRAIT_HDPI  "portrait_hdpi"

namespace mbp
{

class MB_EXPORT Device
{
public:
    /*
     * NOTE: Keep these options synchronized with mbbootui/config/config.{c,h}pp
     */

    enum TwFlags : uint64_t
    {
        FLAG_TW_TOUCHSCREEN_SWAP_XY             = 0x1ull,
        FLAG_TW_TOUCHSCREEN_FLIP_X              = 0x2ull,
        FLAG_TW_TOUCHSCREEN_FLIP_Y              = 0x4ull,
        FLAG_TW_GRAPHICS_FORCE_USE_LINELENGTH   = 0x8ull,
        FLAG_TW_SCREEN_BLANK_ON_BOOT            = 0x10ull,
        FLAG_TW_BOARD_HAS_FLIPPED_SCREEN        = 0x20ull,
        FLAG_TW_IGNORE_MAJOR_AXIS_0             = 0x40ull,
        FLAG_TW_IGNORE_MT_POSITION_0            = 0x80ull,
        FLAG_TW_IGNORE_ABS_MT_TRACKING_ID       = 0x100ull,
        FLAG_TW_NEW_ION_HEAP                    = 0x200ull,
        FLAG_TW_NO_SCREEN_BLANK                 = 0x400ull,
        FLAG_TW_NO_SCREEN_TIMEOUT               = 0x800ull,
        FLAG_TW_ROUND_SCREEN                    = 0x1000ull,
        FLAG_TW_NO_CPU_TEMP                     = 0x2000ull,
        FLAG_TW_QCOM_RTC_FIX                    = 0x4000ull,
        FLAG_TW_HAS_DOWNLOAD_MODE               = 0x8000ull,
        FLAG_TW_PREFER_LCD_BACKLIGHT            = 0x10000ull,
    };

    enum class TwPixelFormat
    {
        DEFAULT,
        ABGR_8888,
        RGBX_8888,
        BGRA_8888,
        RGBA_8888
    };

    enum class TwForcePixelFormat
    {
        NONE,
        RGB_565
    };

    struct TwOptions
    {
        bool supported = false;

        uint64_t flags = 0;

        TwPixelFormat pixelFormat = TwPixelFormat::DEFAULT;
        TwForcePixelFormat forcePixelFormat = TwForcePixelFormat::NONE;

        int overscanPercent = 0;
        int defaultXOffset = 0;
        int defaultYOffset = 0;

        std::string brightnessPath;
        std::string secondaryBrightnessPath;
        int maxBrightness = -1;
        int defaultBrightness = -1;

        std::string batteryPath;
        std::string cpuTempPath;

        std::string inputBlacklist;
        std::string inputWhitelist;

        std::vector<std::string> graphicsBackends;

        std::string theme = TW_THEME_PORTRAIT_HDPI;
    };

    struct CryptoOptions
    {
        bool supported = false;

        std::string headerPath;
    };

    Device();
    ~Device();

    std::string id() const;
    void setId(std::string id);

    std::vector<std::string> codenames() const;
    void setCodenames(std::vector<std::string> names);

    std::string name() const;
    void setName(std::string name);

    std::string architecture() const;
    void setArchitecture(std::string arch);

    std::vector<std::string> blockDevBaseDirs() const;
    void setBlockDevBaseDirs(std::vector<std::string> dirs);

    std::vector<std::string> systemBlockDevs() const;
    void setSystemBlockDevs(std::vector<std::string> blockDevs);

    std::vector<std::string> cacheBlockDevs() const;
    void setCacheBlockDevs(std::vector<std::string> blockDevs);

    std::vector<std::string> dataBlockDevs() const;
    void setDataBlockDevs(std::vector<std::string> blockDevs);

    std::vector<std::string> bootBlockDevs() const;
    void setBootBlockDevs(std::vector<std::string> blockDevs);

    std::vector<std::string> recoveryBlockDevs() const;
    void setRecoveryBlockDevs(std::vector<std::string> blockDevs);

    std::vector<std::string> extraBlockDevs() const;
    void setExtraBlockDevs(std::vector<std::string> blockDevs);

    const TwOptions * twOptions() const;
    TwOptions * twOptions();

    const CryptoOptions * cryptoOptions() const;
    CryptoOptions * cryptoOptions();

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

}
