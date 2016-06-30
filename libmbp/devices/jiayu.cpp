/*
 * Copyright (C) 2015-2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "devices/jiayu.h"

#include "devices/paths.h"

namespace mbp
{

void addJiayuDevices(std::vector<Device *> *devices)
{
    Device *device;

    // Jiayu S3
    device = new Device();
    device->setId("h560");
    device->setArchitecture(ARCH_ARM64_V8A);
    device->setCodenames({ "h560", "s3_h560", "Jiayu_S3", "Jiayu-S3", "JY_S3",
        "JY-S3"});
    device->setName("Jiayu S3");
    device->setBlockDevBaseDirs({ MTK_BASE_DIR, MTK_11230000_BASE_DIR });
    device->setSystemBlockDevs({ MTK_SYSTEM, MTK_11230000_SYSTEM,
        "/dev/block/mmcblk0p17" });
    device->setCacheBlockDevs({ MTK_CACHE, MTK_11230000_CACHE,
        "/dev/block/mmcblk0p18" });
    device->setDataBlockDevs({ MTK_USERDATA, MTK_11230000_USERDATA,
        "/dev/block/mmcblk0p19" });
    device->setBootBlockDevs({ MTK_BOOT, MTK_11230000_BOOT,
        "/dev/block/mmcblk0p7" });
    device->setRecoveryBlockDevs({ MTK_RECOVERY, MTK_11230000_RECOVERY,
        "/dev/block/mmcblk0p8" });
    device->setExtraBlockDevs({
        MTK_LK, MTK_11230000_LK,
        MTK_LOGO, MTK_11230000_LOGO,
        MTK_PARA, MTK_11230000_PARA,
        MTK_TEE1, MTK_11230000_TEE1,
        MTK_TEE2, MTK_11230000_TEE2,
        MTK_UBOOT, MTK_11230000_UBOOT });
    device->twOptions()->supported = true;
    device->twOptions()->flags =
          Device::FLAG_TW_IGNORE_MAJOR_AXIS_0
        | Device::FLAG_TW_PREFER_LCD_BACKLIGHT
        | Device::FLAG_TW_QCOM_RTC_FIX
        | Device::FLAG_TW_GRAPHICS_FORCE_USE_LINELENGTH
        | Device::FLAG_TW_HAS_DOWNLOAD_MODE;
    device->twOptions()->graphicsBackends = { "fbdev" };
    device->twOptions()->pixelFormat =
        Device::TwPixelFormat::RGBX_8888;
    device->twOptions()->brightnessPath =
        "/sys/class/leds/lcd-backlight/brightness";
    device->twOptions()->maxBrightness = 255;
    device->twOptions()->defaultBrightness = 160;
    devices->push_back(device);
}

}
