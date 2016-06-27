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

#include "devices/nexus.h"

#include "devices/paths.h"

namespace mbp
{

void addNexusDevices(std::vector<Device *> *devices)
{
    Device *device;

    // Google/LG Nexus 4
    device = new Device();
    device->setId("mako");
    device->setCodenames({ "mako" });
    device->setName("Google/LG Nexus 4");
    device->setBlockDevBaseDirs({ QCOM_BASE_DIR });
    device->setSystemBlockDevs({ QCOM_SYSTEM, "/dev/block/mmcblk0p21" });
    device->setCacheBlockDevs({ QCOM_CACHE, "/dev/block/mmcblk0p22" });
    device->setDataBlockDevs({ QCOM_USERDATA, "/dev/block/mmcblk0p23" });
    device->setBootBlockDevs({ QCOM_BOOT, "/dev/block/mmcblk0p6" });
    device->setRecoveryBlockDevs({ QCOM_RECOVERY, "/dev/block/mmcblk0p7" });
    devices->push_back(device);

    // Google/LG Nexus 5
    device = new Device();
    device->setId("hammerhead");
    device->setCodenames({ "hammerhead" });
    device->setName("Google/LG Nexus 5");
    device->setBlockDevBaseDirs({ QCOM_BASE_DIR });
    device->setSystemBlockDevs({ QCOM_SYSTEM, "/dev/block/mmcblk0p25" });
    device->setCacheBlockDevs({ QCOM_CACHE, "/dev/block/mmcblk0p27" });
    device->setDataBlockDevs({ QCOM_USERDATA, "/dev/block/mmcblk0p28" });
    device->setBootBlockDevs({ QCOM_BOOT, "/dev/block/mmcblk0p19" });
    device->setExtraBlockDevs({ QCOM_ABOOT, QCOM_IMGDATA, QCOM_MISC, QCOM_MODEM,
                                QCOM_RPM, QCOM_SBL1, QCOM_SDI, QCOM_TZ });
    device->setRecoveryBlockDevs({ QCOM_RECOVERY });
    devices->push_back(device);

    // Google/LG Nexus 5X
    device = new Device();
    device->setId("bullhead");
    device->setArchitecture(ARCH_ARM64_V8A);
    device->setCodenames({ "bullhead" });
    device->setName("Google/LG Nexus 5X");
    device->setBlockDevBaseDirs({ F9824900_SOC0_BASE_DIR,
                                  BOOTDEVICE_BASE_DIR });
    device->setSystemBlockDevs({ F9824900_SOC0_SYSTEM, BOOTDEVICE_SYSTEM,
                                 "/dev/block/mmcblk0p41" });
    device->setCacheBlockDevs({ F9824900_SOC0_CACHE, BOOTDEVICE_CACHE,
                                "/dev/block/mmcblk0p40" });
    device->setDataBlockDevs({ F9824900_SOC0_USERDATA, BOOTDEVICE_USERDATA,
                               "/dev/block/mmcblk0p45" });
    device->setBootBlockDevs({ F9824900_SOC0_BOOT, BOOTDEVICE_BOOT,
                               "/dev/block/mmcblk0p37" });
    device->setRecoveryBlockDevs({ F9824900_SOC0_RECOVERY, BOOTDEVICE_RECOVERY,
                                   "/dev/block/mmcblk0p38" });
    devices->push_back(device);

    // Google/Motorola Nexus 6
    device = new Device();
    device->setId("shamu");
    device->setCodenames({ "shamu" });
    device->setName("Google/Motorola Nexus 6");
    device->setBlockDevBaseDirs({ QCOM_BASE_DIR });
    device->setSystemBlockDevs({ QCOM_SYSTEM, "/dev/block/mmcblk0p41" });
    device->setCacheBlockDevs({ QCOM_CACHE, "/dev/block/mmcblk0p38" });
    device->setDataBlockDevs({ QCOM_USERDATA, "/dev/block/mmcblk0p42" });
    device->setBootBlockDevs({ QCOM_BOOT, "/dev/block/mmcblk0p37" });
    device->setRecoveryBlockDevs({ QCOM_RECOVERY, "/dev/block/mmcblk0p35" });
    devices->push_back(device);

    // Google/Huawei Nexus 6P
    device = new Device();
    device->setId("angler");
    device->setCodenames({ "angler" });
    device->setName("Google/Huawei Nexus 6P");
    device->setArchitecture(ARCH_ARM64_V8A);
    device->setBlockDevBaseDirs({ F9824900_SOC0_BASE_DIR });
    device->setSystemBlockDevs({ BOOTDEVICE_SYSTEM, F9824900_SOC0_SYSTEM,
                                 "/dev/block/mmcblk0p43" });
    device->setCacheBlockDevs({ BOOTDEVICE_CACHE, F9824900_SOC0_CACHE,
                                "/dev/block/mmcblk0p38" });
    device->setDataBlockDevs({ BOOTDEVICE_USERDATA, F9824900_SOC0_USERDATA,
                               "/dev/block/mmcblk0p44" });
    device->setBootBlockDevs({ BOOTDEVICE_BOOT, F9824900_SOC0_BOOT,
                               "/dev/block/mmcblk0p34" });
    device->setRecoveryBlockDevs({ BOOTDEVICE_RECOVERY, F9824900_SOC0_RECOVERY,
                                   "/dev/block/mmcblk0p35" });
    // vendor is 37 if you need it
    devices->push_back(device);

    // Google/ASUS Nexus 7 (2012 Wifi)
    device = new Device();
    device->setId("grouper");
    device->setCodenames({ "grouper" });
    device->setName("Google/ASUS Nexus 7 (2012 Wifi)");
    device->setBlockDevBaseDirs({ TEGRA3_BASE_DIR });
    device->setSystemBlockDevs({ TEGRA3_SYSTEM, "/dev/block/mmcblk0p3" });
    device->setCacheBlockDevs({ TEGRA3_CACHE, "/dev/block/mmcblk0p4" });
    device->setDataBlockDevs({ TEGRA3_USERDATA, "/dev/block/mmcblk0p9" });
    device->setBootBlockDevs({ TEGRA3_BOOT, "/dev/block/mmcblk0p2" });
    device->setRecoveryBlockDevs({ TEGRA3_RECOVERY, "/dev/block/mmcblk0p1" });
    devices->push_back(device);

    // Google/ASUS Nexus 7 (2013 Wifi)
    device = new Device();
    device->setId("flo");
    device->setCodenames({ "flo" });
    device->setName("Google/ASUS Nexus 7 (2013 Wifi)");
    device->setBlockDevBaseDirs({ QCOM_BASE_DIR });
    device->setSystemBlockDevs({ QCOM_SYSTEM, "/dev/block/mmcblk0p22" });
    device->setCacheBlockDevs({ QCOM_CACHE, "/dev/block/mmcblk0p23" });
    device->setDataBlockDevs({ QCOM_USERDATA, "/dev/block/mmcblk0p30" });
    device->setBootBlockDevs({ QCOM_BOOT, "/dev/block/mmcblk0p14" });
    device->setRecoveryBlockDevs({ QCOM_RECOVERY });
    devices->push_back(device);
}

}
