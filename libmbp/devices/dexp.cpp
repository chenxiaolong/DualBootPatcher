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

#include "devices/dexp.h"

#include "devices/paths.h"

namespace mbp
{

void addDexpDevices(std::vector<Device *> *devices)
{
    Device *device;

    // DEXP XLTE4.5
    device = new Device();
    device->setId("XLTE4");
    device->setCodenames({ "XLTE4" });
    device->setName("DEXP X LTE 4.5");
    device->setBlockDevBaseDirs({ BOOTDEVICE_BASE_DIR });
    device->setSystemBlockDevs({ BOOTDEVICE_SYSTEM, "/dev/block/mmcblk0p21" });
    device->setCacheBlockDevs({ BOOTDEVICE_CACHE, "/dev/block/mmcblk0p23" });
    device->setDataBlockDevs({ BOOTDEVICE_USERDATA, "/dev/block/mmcblk0p25" });
    device->setBootBlockDevs({ BOOTDEVICE_BOOT, "/dev/block/mmcblk0p20" });
    device->setRecoveryBlockDevs({ BOOTDEVICE_RECOVERY, "/dev/block/mmcblk0p24" });
    devices->push_back(device);

    // DEXP MLTE5
    device = new Device();
    device->setId("mlte5");
    device->setCodenames({ "MLTE5" });
    device->setName("DEXP M LTE 5");
    device->setBlockDevBaseDirs({ BOOTDEVICE_BASE_DIR });
    device->setSystemBlockDevs({ BOOTDEVICE_SYSTEM, "/dev/block/mmcblk0p21" });
    device->setCacheBlockDevs({ BOOTDEVICE_CACHE, "/dev/block/mmcblk0p23" });
    device->setDataBlockDevs({ BOOTDEVICE_USERDATA, "/dev/block/mmcblk0p25" });
    device->setBootBlockDevs({ BOOTDEVICE_BOOT, "/dev/block/mmcblk0p20" });
    device->setRecoveryBlockDevs({ BOOTDEVICE_RECOVERY, "/dev/block/mmcblk0p24" });
    devices->push_back(device);
}

}
