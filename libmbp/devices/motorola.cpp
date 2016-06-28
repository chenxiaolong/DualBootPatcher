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
    device->twOptions()->supported = true;
    device->twOptions()->flags =
            Device::FLAG_TW_IGNORE_MAJOR_AXIS_0
            | Device::FLAG_TW_QCOM_RTC_FIX;
    device->twOptions()->graphicsBackends = { "overlay_msm_old", "fbdev" };
    devices->push_back(device);

    // Motorola Moto G (2014)
    device = new Device();
    device->setId("titan");
    device->setCodenames({ "titan_umts", "titan_umtsds", "titan_umts", "titan_udstv", "xt1063", "xt1068", "xt1064", "xt1069" });
    device->setName("Motorola Moto G (2014)");
    device->setBlockDevBaseDirs({ BOOTDEVICE_BASE_DIR, QCOM_BASE_DIR });
    device->setSystemBlockDevs({ BOOTDEVICE_SYSTEM, QCOM_SYSTEM, "/dev/block/mmcblk0p36" });
    device->setCacheBlockDevs({ BOOTDEVICE_CACHE, QCOM_CACHE, "/dev/block/mmcblk0p35" });
    device->setDataBlockDevs({ BOOTDEVICE_USERDATA, QCOM_USERDATA, "/dev/block/mmcblk0p38" });
    device->setBootBlockDevs({ BOOTDEVICE_BOOT, QCOM_BOOT, "/dev/block/mmcblk0p31" });
    device->setRecoveryBlockDevs({ BOOTDEVICE_RECOVERY, QCOM_RECOVERY, "/dev/block/mmcblk0p32" });
    devices->push_back(device);

    // Motorola Moto G (2015)
    device = new Device();
    device->setId("osprey");
    device->setCodenames({ "osprey", "osprey_u2" });
    device->setName("Motorola Moto G (2015)");
    device->setBlockDevBaseDirs({ SOC0_BASE_DIR });
    device->setSystemBlockDevs({ SOC0_SYSTEM, "/dev/block/mmcblk0p41" });
    device->setCacheBlockDevs({ SOC0_CACHE, "/dev/block/mmcblk0p40" });
    device->setDataBlockDevs({ SOC0_USERDATA, "/dev/block/mmcblk0p42" });
    device->setBootBlockDevs({ SOC0_BOOT, "/dev/block/mmcblk0p31" });
    device->setRecoveryBlockDevs({ SOC0_RECOVERY, "/dev/block/mmcblk0p32" });
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

    // Motorola Moto E (2nd gen 3G)
    device = new Device();
    device->setId("otus");
    device->setCodenames({ "otus", "otus_ds", "otus", "xt1505", "xt1506", "xt1511" });
    device->setName("Motorola Moto E (2nd gen 3G)");
    device->setBlockDevBaseDirs({ QCOM_BASE_DIR });
    device->setSystemBlockDevs({ QCOM_SYSTEM, "/dev/block/mmcblk0p39" });
    device->setCacheBlockDevs({ QCOM_CACHE, "/dev/block/mmcblk0p40" });
    device->setDataBlockDevs({ QCOM_USERDATA, "/dev/block/mmcblk0p41" });
    device->setBootBlockDevs({ QCOM_BOOT, "/dev/block/mmcblk0p32" });
    device->setRecoveryBlockDevs({ QCOM_RECOVERY, "/dev/block/mmcblk0p33" });
    devices->push_back(device);

    // Motorola Moto E (2nd gen 4G)
    device = new Device();
    device->setId("surnia");
    device->setCodenames({ "surnia", "surnia_cdma", "surnia_boost", "surnia_verizon", "surnia_umts",
                           "surnia_cricket", "surnia_retus", "surnia_tefla", "xt1514",
                           "xt1521", "xt1523", "xt1524", "xt1526", "xt1527" });
    device->setName("Motorola Moto E (2nd gen 4G)");
    device->setBlockDevBaseDirs({ QCOM_BASE_DIR });
    device->setSystemBlockDevs({ QCOM_SYSTEM, "/dev/block/mmcblk0p41" });
    device->setCacheBlockDevs({ QCOM_CACHE, "/dev/block/mmcblk0p42" });
    device->setDataBlockDevs({ QCOM_USERDATA, "/dev/block/mmcblk0p43" });
    device->setBootBlockDevs({ QCOM_BOOT, "/dev/block/mmcblk0p33" });
    device->setRecoveryBlockDevs({ QCOM_RECOVERY, "/dev/block/mmcblk0p34" });
    devices->push_back(device);

    // Motorola Moto X (2013)
    device = new Device();
    device->setId("ghost");
    device->setCodenames({ "ghost", "ghost_att", "ghost_rcica", "ghost_retail",
                           "ghost_sprint", "ghost_usc", "ghost_verizon",
                           "xt1052", "xt1053", "xt1055", "xt1056", "xt1058",
                           "xt1060" });
    device->setName("Motorola Moto X (2013)");
    device->setBlockDevBaseDirs({ QCOM_BASE_DIR });
    device->setSystemBlockDevs({ QCOM_SYSTEM, "/dev/block/mmcblk0p38" });
    device->setCacheBlockDevs({ QCOM_CACHE, "/dev/block/mmcblk0p36" });
    device->setDataBlockDevs({ QCOM_USERDATA, "/dev/block/mmcblk0p40" });
    device->setBootBlockDevs({ QCOM_BOOT, "/dev/block/mmcblk0p33" });
    device->setRecoveryBlockDevs({ QCOM_RECOVERY });
    devices->push_back(device);

    // Motorola Moto X (2014)
    device = new Device();
    device->setId("victara");
    device->setCodenames({ "victara" });
    device->setName("Motorola Moto X (2014)");
    device->setBlockDevBaseDirs({ QCOM_BASE_DIR });
    device->setSystemBlockDevs({ QCOM_SYSTEM, "/dev/block/mmcblk0p38" });
    device->setCacheBlockDevs({ QCOM_CACHE, "/dev/block/mmcblk0p37" });
    device->setDataBlockDevs({ QCOM_USERDATA, "/dev/block/mmcblk0p39" });
    device->setBootBlockDevs({ QCOM_BOOT, "/dev/block/mmcblk0p33" });
    device->setRecoveryBlockDevs({ QCOM_RECOVERY, "/dev/block/mmcblk0p34" });
    devices->push_back(device);

    // Motorola Moto X Pure Edition
    device = new Device();
    device->setId("clark");
    device->setCodenames({ "clark" });
    device->setName("Motorola Moto X Pure Edition");
    device->setArchitecture(ARCH_ARM64_V8A);
    device->setBlockDevBaseDirs({ BOOTDEVICE_BASE_DIR, F9824900_BASE_DIR,
                                  F9824900_SOC0_BASE_DIR });
    device->setSystemBlockDevs({ BOOTDEVICE_SYSTEM, F9824900_SYSTEM,
                                 F9824900_SOC0_SYSTEM,
                                 "/dev/block/mmcblk0p46" });
    device->setCacheBlockDevs({ BOOTDEVICE_CACHE, F9824900_CACHE,
                                F9824900_SOC0_CACHE,
                                "/dev/block/mmcblk0p45" });
    device->setDataBlockDevs({ BOOTDEVICE_USERDATA, F9824900_USERDATA,
                               F9824900_SOC0_USERDATA,
                               "/dev/block/mmcblk0p47" });
    device->setBootBlockDevs({ BOOTDEVICE_BOOT, F9824900_BOOT,
                               F9824900_SOC0_BOOT,
                               "/dev/block/mmcblk0p36" });
    device->setRecoveryBlockDevs({ BOOTDEVICE_RECOVERY, F9824900_RECOVERY,
                                   F9824900_SOC0_RECOVERY,
                                   "/dev/block/mmcblk0p37" });
    devices->push_back(device);
}

}
