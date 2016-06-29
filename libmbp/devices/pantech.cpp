/*
 * Copyright (C) 2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "devices/pantech.h"

#include "devices/paths.h"

namespace mbp
{
    void addPantechDevices(std::vector<Device *> *devices)
    {
        Device *device;

        // PANTECH VEGA IRON
        device = new Device();
        device->setId("A910");
        device->setCodenames({ "ef63s", "ef63l", "ef63k" });
        device->setName("VEGA IRON 2");
        device->setBlockDevBaseDirs({ QCOM_BASE_DIR });
        device->setSystemBlockDevs({ QCOM_SYSTEM, "/dev/block/mmcblk0p15" });
        device->setCacheBlockDevs({ QCOM_CACHE, "/dev/block/mmcblk0p17" });
        device->setDataBlockDevs({ QCOM_USERDATA, "/dev/block/mmcblk0p29" });
        device->setBootBlockDevs({ QCOM_BOOT, "/dev/block/mmcblk0p7" });
        device->setRecoveryBlockDevs({ QCOM_RECOVERY, "/dev/block/mmcblk0p14" });
        device->setExtraBlockDevs({ QCOM_TZ, "/dev/block/mmcblk0p8" });
        device->twOptions()->supported = true;
        device->twOptions()->flags =
                Device::FLAG_TW_QCOM_RTC_FIX | Device::FLAG_TW_GRAPHICS_FORCE_USE_LINELENGTH;
        device->twOptions()->graphicsBackends = { "overlay" };
        device->twOptions()->pixelFormat = Device::TwPixelFormat::RGBX_8888;
        device->twOptions()->brightnessPath = "/sys/class/leds/lcd-backlight/brightness";
        device->twOptions()->maxBrightness = 255;
        device->twOptions()->defaultBrightness = 160;
        devices->push_back(device);

        // PANTECH VEGA IRON
        device = new Device();
        device->setId("A870");
        device->setCodenames({ "ef52s", "ef52l", "ef52k" });
        device->setName("VEGA IRON 2");
        device->setBlockDevBaseDirs({ QCOM_BASE_DIR });
        device->setSystemBlockDevs({ QCOM_SYSTEM, "/dev/block/mmcblk0p14" });
        device->setCacheBlockDevs({ QCOM_CACHE, "/dev/block/mmcblk0p16" });
        device->setDataBlockDevs({ QCOM_USERDATA, "/dev/block/mmcblk0p26" });
        device->setBootBlockDevs({ QCOM_BOOT, "/dev/block/mmcblk0p7" });
        device->setRecoveryBlockDevs({ QCOM_RECOVERY, "/dev/block/mmcblk0p19" });
        device->setExtraBlockDevs({ QCOM_TZ, "/dev/block/mmcblk0p8" });
        devices->push_back(device);
    }
}

