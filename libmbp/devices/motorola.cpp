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

#include "devices/motorola.h"

#include "devices/paths.h"

namespace mbp
{

void addMotorolaDevices(std::vector<Device *> *devices)
{
    Device *device;

    // Motorola Moto G (2013)
    device = new Device();
    device->setId("falcon");
    device->setCodenames({ "falcon", "falcon_umts", "falcon_umtsds",
                           "xt1032" });
    device->setName("Motorola Moto G (2013)");
    device->setBlockDevBaseDirs({ QCOM_BASE_DIR });
    device->setSystemBlockDevs({ QCOM_SYSTEM, "/dev/block/mmcblk0p34" });
    device->setCacheBlockDevs({ QCOM_CACHE, "/dev/block/mmcblk0p33" });
    device->setDataBlockDevs({ QCOM_USERDATA, "/dev/block/mmcblk0p36" });
    device->setBootBlockDevs({ QCOM_BOOT, "/dev/block/mmcblk0p31" });
    device->setRecoveryBlockDevs({ QCOM_RECOVERY, "/dev/block/mmcblk0p32" });
    devices->push_back(device);

    // Motorola Moto E (1st gen)
    device = new Device();
    device->setId("condor");
    device->setCodenames({ "condor", "condor_umts", "condor_umtsds", "xt1022", "xt1021", "xt1023" });
    device->setName("Motorola Moto E (1st gen)");
    device->setBlockDevBaseDirs({ QCOM_BASE_DIR });
    device->setSystemBlockDevs({ QCOM_SYSTEM, "/dev/block/mmcblk0p34" });
    device->setCacheBlockDevs({ QCOM_CACHE, "/dev/block/mmcblk0p33" });
    device->setDataBlockDevs({ QCOM_USERDATA, "/dev/block/mmcblk0p36" });
    device->setBootBlockDevs({ QCOM_BOOT, "/dev/block/mmcblk0p31" });
    device->setRecoveryBlockDevs({ QCOM_RECOVERY, "/dev/block/mmcblk0p32" });
    devices->push_back(device);

    // Motorola Moto X (2013)
    device = new Device();
    device->setId("ghost");
    device->setCodenames({ "ghost", "ghost_att", "ghost_rcica", "ghost_retail",
                           "ghost_sprint", "ghost_usc", "ghost_verizon",
                           "xt1052", "xt1053", "xt1055", "xt1056", "xt1058",
                           "xt1060", });
    device->setName("Motorola Moto X (2013)");
    device->setBlockDevBaseDirs({ QCOM_BASE_DIR });
    device->setSystemBlockDevs({ QCOM_SYSTEM, "/dev/block/mmcblk0p38" });
    device->setCacheBlockDevs({ QCOM_CACHE, "/dev/block/mmcblk0p36" });
    device->setDataBlockDevs({ QCOM_USERDATA, "/dev/block/mmcblk0p40" });
    device->setBootBlockDevs({ QCOM_BOOT, "/dev/block/mmcblk0p33" });
    device->setRecoveryBlockDevs({ QCOM_RECOVERY });
    devices->push_back(device);
}

}