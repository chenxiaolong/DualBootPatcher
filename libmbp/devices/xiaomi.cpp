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

    // Xiaomi Redmi Note 2
    device = new Device();
    device->setId("hermes");
    device->setCodenames({ "hermes" });
    device->setName("Xiaomi Redmi Note 2");
    device->setArchitecture(ARCH_ARM64_V8A);
    device->setBlockDevBaseDirs({ BOOTDEVICE_BASE_DIR, MTK_BASE_DIR });
    device->setSystemBlockDevs({ BOOTDEVICE_SYSTEM, MTK_SYSTEM,
                                 "/dev/block/mmcblk0p15" });
    device->setCacheBlockDevs({ BOOTDEVICE_CACHE, MTK_CACHE,
                                "/dev/block/mmcblk0p16" });
    device->setDataBlockDevs({ BOOTDEVICE_USERDATA, MTK_USERDATA,
                               "/dev/block/mmcblk0p17" });
    device->setBootBlockDevs({ BOOTDEVICE_BOOT, MTK_BOOT,
                               "/dev/block/mmcblk0p7" });
    device->setRecoveryBlockDevs({ BOOTDEVICE_RECOVERY, MTK_RECOVERY,
                                   "/dev/block/mmcblk0p8" });
    device->setExtraBlockDevs({
        // Directly written by updater-script
        MTK_LK, "/dev/block/mmcblk0p6",
        MTK_LOGO, "/dev/block/mmcblk0p11",
        // From reverse engineering the update-binary, it appears that
        // /dev/block/mmcblk0boot0 is the "preloader" partition
        "/dev/block/mmcblk0boot0"
    });
    devices->push_back(device);

    // Xiaomi Redmi Note 3 (MTK)
    device = new Device();
    device->setId("hennessy");
    device->setCodenames({ "hennessy" });
    device->setName("Xiaomi Redmi Note 3 (MTK)");
    device->setArchitecture(ARCH_ARM64_V8A);
    device->setBlockDevBaseDirs({ MTK_BASE_DIR });
    device->setSystemBlockDevs({ MTK_SYSTEM, "/dev/block/mmcblk0p15" });
    device->setCacheBlockDevs({ MTK_CACHE, "/dev/block/mmcblk0p16" });
    device->setDataBlockDevs({ MTK_USERDATA, "/dev/block/mmcblk0p17" });
    device->setBootBlockDevs({ MTK_BOOT, "/dev/block/mmcblk0p7" });
    device->setRecoveryBlockDevs({ MTK_RECOVERY, "/dev/block/mmcblk0p8" });
    device->setExtraBlockDevs({
        // write_raw_image() callback in update-binary checks if the para
        // partition exists
        MTK_PARA, "/dev/block/mmcblk0p10",
        // From reverse engineering the update-binary, it appears that
        // /dev/block/mmcblk0boot0 is the "preloader" partition
        "/dev/block/mmcblk0boot0"
    });
    devices->push_back(device);
}

}
