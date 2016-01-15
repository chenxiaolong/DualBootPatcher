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

#include "devices/xiaomi.h"

#include "devices/paths.h"

namespace mbp
{
    void addXiaomiDevices(std::vector<Device *> *devices)
    {
        Device *device;

        // Xiaomi Redmi 1s
        device = new Device();
        device->setId("armani");
        device->setCodenames({ "armani" });
        device->setName("Xiaomi HM 1S");
        device->setBlockDevBaseDirs({ QCOM_BASE_DIR });
        device->setSystemBlockDevs({ QCOM_SYSTEM, "/dev/block/mmcblk0p27" });
        device->setCacheBlockDevs({ QCOM_CACHE, "/dev/block/mmcblk0p28" });
        device->setDataBlockDevs({ QCOM_USERDATA, "/dev/block/mmcblk0p29" });
        device->setBootBlockDevs({ QCOM_BOOT, "/dev/block/mmcblk0p24" });
        device->setRecoveryBlockDevs({ QCOM_RECOVERY, "/dev/block/mmcblk0p25" });
        devices->push_back(device);
    }
}
