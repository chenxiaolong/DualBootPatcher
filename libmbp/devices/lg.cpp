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

#include "devices/lg.h"

#include "devices/paths.h"

namespace mbp
{

void addLgOptimusGSeriesPhones(std::vector<Device *> *devices)
{
    Device *device;

    // LG G2
    device = new Device();
    device->setId("lgg2");
    device->setCodenames({ "g2", "d800", "d801", "ls980", "vs980" });
    device->setName("LG G2");
    device->setBlockDevBaseDirs({ QCOM_BASE_DIR });
    device->setSystemBlockDevs({ QCOM_SYSTEM, "/dev/block/mmcblk0p34" });
    device->setCacheBlockDevs({ QCOM_CACHE, "/dev/block/mmcblk0p35" });
    device->setDataBlockDevs({ QCOM_USERDATA, "/dev/block/mmcblk0p38" });
    device->setBootBlockDevs({ QCOM_BOOT, "/dev/block/mmcblk0p7" });
    device->setRecoveryBlockDevs({ QCOM_RECOVERY, "/dev/block/mmcblk0p15" });
    device->setExtraBlockDevs({ QCOM_ABOOT, QCOM_TZ });
    devices->push_back(device);

    // LG G2 (d802)
    device = new Device();
    device->setId("lgg2_d802");
    device->setCodenames({ "g2", "d802", "d802t" });
    device->setName("LG G2 (d802)");
    device->setBlockDevBaseDirs({ QCOM_BASE_DIR });
    device->setSystemBlockDevs({ QCOM_SYSTEM, "/dev/block/mmcblk0p30" });
    device->setCacheBlockDevs({ QCOM_CACHE, "/dev/block/mmcblk0p31" });
    device->setDataBlockDevs({ QCOM_USERDATA, "/dev/block/mmcblk0p35" });
    device->setBootBlockDevs({ QCOM_BOOT, "/dev/block/mmcblk0p7" });
    device->setRecoveryBlockDevs({ QCOM_RECOVERY, "/dev/block/mmcblk0p15" });
    device->setExtraBlockDevs({ QCOM_ABOOT, QCOM_TZ });
    devices->push_back(device);

    // LG G3
    device = new Device();
    device->setId("lgg3");
    device->setCodenames({ "g3", "d850", "d851", "d852", "d855", "f400",
                           "f400k", "ls990", "vs985" });
    device->setName("LG G3");
    device->setBlockDevBaseDirs({ QCOM_BASE_DIR });
    device->setSystemBlockDevs({ QCOM_SYSTEM, "/dev/block/mmcblk0p40" });
    device->setCacheBlockDevs({ QCOM_CACHE, "/dev/block/mmcblk0p41" });
    device->setDataBlockDevs({ QCOM_USERDATA, "/dev/block/mmcblk0p43" });
    device->setBootBlockDevs({ QCOM_BOOT, "/dev/block/mmcblk0p18" });
    device->setRecoveryBlockDevs({ QCOM_RECOVERY });
    device->setExtraBlockDevs({ QCOM_ABOOT, QCOM_MODEM });
    devices->push_back(device);

    // LG G4
    device = new Device();
    device->setId("lgg4");
    device->setArchitecture(ARCH_ARM64_V8A);
    device->setCodenames({ "p1", "h815" });
    device->setName("LG G4");
    device->setBlockDevBaseDirs({ F9824900_BASE_DIR, BOOTDEVICE_BASE_DIR });
    device->setSystemBlockDevs({ F9824900_SYSTEM, BOOTDEVICE_SYSTEM,
                                 "/dev/block/mmcblk0p47" });
    device->setCacheBlockDevs({ F9824900_CACHE, BOOTDEVICE_CACHE,
                                "/dev/block/mmcblk0p49" });
    device->setDataBlockDevs({ F9824900_USERDATA, BOOTDEVICE_USERDATA,
                               "/dev/block/mmcblk0p50" });
    device->setBootBlockDevs({ F9824900_BOOT, BOOTDEVICE_BOOT,
                               "/dev/block/mmcblk0p38" });
    device->setRecoveryBlockDevs({ F9824900_RECOVERY, BOOTDEVICE_RECOVERY,
                                   "/dev/block/mmcblk0p39" });
    devices->push_back(device);
}

void addLgOptimusLSeriesPhones(std::vector<Device *> *devices)
{
    Device *device;

    // LG L3 II
    device = new Device();
    device->setId("vee3");
    device->setCodenames({ "vee3", "vee3ds", "e425", "e430", "e431", "e435",
        "E425", "E430", "E431", "E435" });
    device->setName("LG L3 II");
    device->setSystemBlockDevs({ "/dev/block/mmcblk0p14",
        "/dev/block/platform/msm_sdcc.3/by-num/p14" });
    device->setCacheBlockDevs({ "/dev/block/mmcblk0p16",
        "/dev/block/platform/msm_sdcc.3/by-num/p16" });
    device->setDataBlockDevs({ "/dev/block/mmcblk0p20",
        "/dev/block/platform/msm_sdcc.3/by-num/p20" });
    device->setBootBlockDevs({ "/dev/block/mmcblk0p9",
        "/dev/block/platform/msm_sdcc.3/by-num/p9" });
    device->setRecoveryBlockDevs({ "/dev/block/mmcblk0p17",
         "/dev/block/platform/msm_sdcc.3/by-num/p17" });
    device->twOptions()->supported = true;
    device->twOptions()->flags =
        Device::FLAG_TW_QCOM_RTC_FIX
        | Device::FLAG_TW_NO_CPU_TEMP
        | Device::FLAG_TW_GRAPHICS_FORCE_USE_LINELENGTH
        | Device::FLAG_TW_PREFER_LCD_BACKLIGHT;
    device->twOptions()->graphicsBackends = { "fbdev" };
    device->twOptions()->pixelFormat = Device::TwPixelFormat::RGBX_8888;
    devices->push_back(device);

    // LG L5
    device = new Device();
    device->setId("m4");
    device->setCodenames({ "m4", "e610", "e612", "e617", "E610", "E612", "E617" });
    device->setName("LG L5");
    device->setSystemBlockDevs({ "/dev/block/mmcblk0p14", "/dev/block/platform/msm_sdcc.3/by-num/p14" });
    device->setCacheBlockDevs({ "/dev/block/mmcblk0p16", "/dev/block/platform/msm_sdcc.3/by-num/p16" });
    device->setDataBlockDevs({ "/dev/block/mmcblk0p20", "/dev/block/platform/msm_sdcc.3/by-num/p20" });
    device->setBootBlockDevs({ "/dev/block/mmcblk0p9", "/dev/block/platform/msm_sdcc.3/by-num/p9" });
    device->setRecoveryBlockDevs({ "/dev/block/mmcblk0p17", "/dev/block/platform/msm_sdcc.3/by-num/p17" });
    devices->push_back(device);

    // LG L7
    device = new Device();
    device->setId("u0");
    device->setCodenames({ "u0", "p700", "p705", "p708", "P700", "P705", "P708" });
    device->setName("LG L7");
    device->setSystemBlockDevs({ "/dev/block/mmcblk0p14", "/dev/block/platform/msm_sdcc.3/by-num/p14" });
    device->setCacheBlockDevs({ "/dev/block/mmcblk0p16", "/dev/block/platform/msm_sdcc.3/by-num/p16" });
    device->setDataBlockDevs({ "/dev/block/mmcblk0p20", "/dev/block/platform/msm_sdcc.3/by-num/p20" });
    device->setBootBlockDevs({ "/dev/block/mmcblk0p9", "/dev/block/platform/msm_sdcc.3/by-num/p9" });
    device->setRecoveryBlockDevs({ "/dev/block/mmcblk0p17", "/dev/block/platform/msm_sdcc.3/by-num/p17" });
    devices->push_back(device);

    // LG L7 II
    device = new Device();
    device->setId("vee7");
    device->setCodenames({ "vee7", "vee7ds", "p710", "p712", "p713", "p714", "p715", "p716",
                           "P710", "P712", "P713", "P714", "P715", "P716" });
    device->setName("LG L7 II");
    device->setSystemBlockDevs({ "/dev/block/mmcblk0p14", "/dev/block/platform/msm_sdcc.3/by-num/p14" });
    device->setCacheBlockDevs({ "/dev/block/mmcblk0p16", "/dev/block/platform/msm_sdcc.3/by-num/p16" });
    device->setDataBlockDevs({ "/dev/block/mmcblk0p20", "/dev/block/platform/msm_sdcc.3/by-num/p20" });
    device->setBootBlockDevs({ "/dev/block/mmcblk0p9", "/dev/block/platform/msm_sdcc.3/by-num/p9" });
    device->setRecoveryBlockDevs({ "/dev/block/mmcblk0p17", "/dev/block/platform/msm_sdcc.3/by-num/p17" });
    devices->push_back(device);
}

void addLgDevices(std::vector<Device *> *devices)
{
    addLgOptimusGSeriesPhones(devices);
    addLgOptimusLSeriesPhones(devices);
}

}
