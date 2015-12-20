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

#include "devices/oneplus.h"

#include "devices/paths.h"

namespace mbp
{

void addOnePlusDevices(std::vector<Device *> *devices)
{
    Device *device;

    // OnePlus One
    device = new Device();
    device->setId("bacon");
    device->setCodenames({ "bacon", "A0001" });
    device->setName("OnePlus One");
    device->setBlockDevBaseDirs({ QCOM_BASE_DIR });
    device->setSystemBlockDevs({ QCOM_SYSTEM, "/dev/block/mmcblk0p14" });
    device->setCacheBlockDevs({ QCOM_CACHE, "/dev/block/mmcblk0p16" });
    device->setDataBlockDevs({ QCOM_USERDATA, "/dev/block/mmcblk0p28" });
    device->setBootBlockDevs({ QCOM_BOOT, "/dev/block/mmcblk0p7" });
    device->setRecoveryBlockDevs({ QCOM_RECOVERY });
    device->setExtraBlockDevs({ QCOM_TZ, "/dev/block/mmcblk0p8" });
    devices->push_back(device);

    // OnePlus Two
    device = new Device();
    device->setId("OnePlus2");
    device->setArchitecture(ARCH_ARM64_V8A);
    device->setCodenames({ "OnePlus2" });
    device->setName("OnePlus Two");
    device->setBlockDevBaseDirs({ F9824900_BASE_DIR, BOOTDEVICE_BASE_DIR });
    device->setSystemBlockDevs({ F9824900_SYSTEM, BOOTDEVICE_SYSTEM,
                                 "/dev/block/mmcblk0p42" });
    device->setCacheBlockDevs({ F9824900_CACHE, BOOTDEVICE_CACHE,
                                "/dev/block/mmcblk0p41" });
    device->setDataBlockDevs({ F9824900_USERDATA, BOOTDEVICE_USERDATA,
                               "/dev/block/mmcblk0p43" });
    device->setBootBlockDevs({ F9824900_BOOT, BOOTDEVICE_BOOT,
                               "/dev/block/mmcblk0p35" });
    device->setRecoveryBlockDevs({ F9824900_RECOVERY, BOOTDEVICE_RECOVERY,
                                   "/dev/block/mmcblk0p36" });
    devices->push_back(device);
}

}