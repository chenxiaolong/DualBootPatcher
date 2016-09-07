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

#include "devices/wileyfox.h"

#include "devices/paths.h"

namespace mbp
{

void addWileyfoxDevices(std::vector<Device *> *devices)
{
    Device *device;

    // WileyFox Swift
    device = new Device();
    device->setId("crackling");
    device->setCodenames({ "crackling" });
    device->setName("Wileyfox Swift");
    device->setBlockDevBaseDirs({ BOOTDEVICE_BASE_DIR });
    device->setSystemBlockDevs({ BOOTDEVICE_SYSTEM, "/dev/block/mmcblk0p25" });
    device->setCacheBlockDevs({ BOOTDEVICE_CACHE, "/dev/block/mmcblk0p29" });
    device->setDataBlockDevs({ BOOTDEVICE_USERDATA, "/dev/block/mmcblk0p31" });
    device->setBootBlockDevs({ BOOTDEVICE_BOOT, "/dev/block/mmcblk0p24" });
    device->setRecoveryBlockDevs({ BOOTDEVICE_RECOVERY, "/dev/block/mmcblk0p26" });
    device->twOptions()->supported = true;
    device->twOptions()->graphicsBackends = { "fbdev" };
    device->twOptions()->flags = Device::FLAG_TW_QCOM_RTC_FIX;
    device->twOptions()->pixelFormat = Device::TwPixelFormat::RGBX_8888;
    device->twOptions()->brightnessPath = "/sys/class/leds/lcd-backlight/brightness";
    device->twOptions()->maxBrightness = 255;
    device->twOptions()->defaultBrightness = 162;
    devices->push_back(device);

    // WileyFox Storm
    device = new Device();
    device->setId("kipper");
    device->setCodenames({ "kipper" });
    device->setName("Wileyfox Storm");
    device->setBlockDevBaseDirs({ BOOTDEVICE_BASE_DIR });
    device->setSystemBlockDevs({ BOOTDEVICE_SYSTEM, "/dev/block/mmcblk0p22" });
    device->setCacheBlockDevs({ BOOTDEVICE_CACHE, "/dev/block/mmcblk0p23" });
    device->setDataBlockDevs({ BOOTDEVICE_USERDATA, "/dev/block/mmcblk0p30" });
    device->setBootBlockDevs({ BOOTDEVICE_BOOT, "/dev/block/mmcblk0p20" });
    device->setRecoveryBlockDevs({ BOOTDEVICE_RECOVERY, "/dev/block/mmcblk0p21" });
    devices->push_back(device);
}

}
