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

#include "devices/lenovo.h"

#include "devices/paths.h"

namespace mbp
{

void addLenovoDevices(std::vector<Device *> *devices)
{
    Device *device;

    // Lenovo K3 Note
    device = new Device();
    device->setId("k50");
    device->setArchitecture(ARCH_ARM64_V8A);
    device->setCodenames({ "K50", "K50a40", "K50t5", "K50-T5", "aio_otfp" });
    device->setName("Lenovo K3 Note");
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
    device->setExtraBlockDevs({ "/dev/block/mmcblk0boot0",
        MTK_LK, MTK_11230000_LK,
        MTK_LOGO, MTK_11230000_LOGO,
        MTK_PARA, MTK_11230000_PARA,
        MTK_TEE1, MTK_11230000_TEE1,
        MTK_TEE2, MTK_11230000_TEE2,
        MTK_UBOOT, MTK_11230000_UBOOT });
    device->twOptions()->supported = true;
    device->twOptions()->graphicsBackends = { "fbdev" };
    device->twOptions()->flags = Device::FLAG_TW_GRAPHICS_FORCE_USE_LINELENGTH;
    device->twOptions()->pixelFormat = Device::TwPixelFormat::RGBX_8888;
    device->twOptions()->maxBrightness = 255;
    device->twOptions()->defaultBrightness = 162;
    device->twOptions()->cpuTempPath = "/sys/class/thermal/thermal_zone1/temp";
    devices->push_back(device);

    // Lenovo Vibe Z2 Pro
    device = new Device();
    device->setId("K920");
    device->setCodenames({ "kingdom_row", "kingdomt" });
    device->setName("Lenovo Vibe Z2 Pro");
    device->setBlockDevBaseDirs({ QCOM_BASE_DIR, BOOTDEVICE_BASE_DIR });
    device->setSystemBlockDevs({ QCOM_SYSTEM, BOOTDEVICE_SYSTEM,
        "/dev/block/mmcblk0p21" });
    device->setCacheBlockDevs({ QCOM_CACHE, BOOTDEVICE_CACHE,
        "/dev/block/mmcblk0p20" });
    device->setDataBlockDevs({ QCOM_USERDATA, BOOTDEVICE_USERDATA,
        "/dev/block/mmcblk0p23" });
    device->setBootBlockDevs({ QCOM_BOOT, BOOTDEVICE_BOOT,
        "/dev/block/mmcblk0p9" });
    device->setRecoveryBlockDevs({ QCOM_RECOVERY, BOOTDEVICE_RECOVERY,
        "/dev/block/mmcblk0p10" });
    devices->push_back(device);

    // Lenovo ZUK Z1
    device = new Device();
    device->setId("Z1");
    device->setCodenames({ "Z1" });
    device->setName("Lenovo ZUK Z1");
    device->setBlockDevBaseDirs({ QCOM_BASE_DIR, BOOTDEVICE_BASE_DIR });
    device->setSystemBlockDevs({ QCOM_SYSTEM, BOOTDEVICE_SYSTEM,
        "/dev/block/mmcblk0p22" });
    device->setCacheBlockDevs({ QCOM_CACHE, BOOTDEVICE_CACHE,
        "/dev/block/mmcblk0p21" });
    device->setDataBlockDevs({ QCOM_USERDATA, BOOTDEVICE_USERDATA,
        "/dev/block/mmcblk0p23" });
    device->setBootBlockDevs({ QCOM_BOOT, BOOTDEVICE_BOOT,
        "/dev/block/mmcblk0p9" });
    device->setRecoveryBlockDevs({ QCOM_RECOVERY, BOOTDEVICE_RECOVERY,
        "/dev/block/mmcblk0p10" });
    devices->push_back(device);
}

}
