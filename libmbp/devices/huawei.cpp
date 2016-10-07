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

#include "devices/huawei.h"

#include "devices/paths.h"

namespace mbp
{

void addHuaweiDevices(std::vector<Device *> *devices)
{
    Device *device;

    // Huawei Ascend P7
    device = new Device();
    device->setId("hwp7");
    device->setCodenames({ "hwp7" });
    device->setName("Huawei Ascend P7");
    device->setBlockDevBaseDirs({ HISILICON_BASE_DIR });
    device->setSystemBlockDevs({ HISILICON_SYSTEM, "/dev/block/mmcblk0p28" });
    device->setCacheBlockDevs({ HISILICON_CACHE, "/dev/block/mmcblk0p21" });
    device->setDataBlockDevs({ HISILICON_USERDATA, "/dev/block/mmcblk0p30" });
    device->setBootBlockDevs({ HISILICON_BOOT, "/dev/block/mmcblk0p17" });
    device->setRecoveryBlockDevs({ HISILICON_RECOVERY,
        "/dev/block/mmcblk0p18" });
    devices->push_back(device);

    // Huawei Ascend Mate 2
    device = new Device();
    device->setId("mt2l03");
    device->setCodenames({ "hwMT2L03", "hwMT2LO3", "mt2", "MT2", "mt2l03",
        "MT2L03", "mt2-l03", "MT2-L03" });
    device->setName("Huawei Ascend Mate 2");
    device->setBlockDevBaseDirs({ QCOM_BASE_DIR });
    device->setSystemBlockDevs({ QCOM_SYSTEM, "/dev/block/mmcblk0p23" });
    device->setCacheBlockDevs({ QCOM_CACHE, "/dev/block/mmcblk0p21" });
    device->setDataBlockDevs({ QCOM_USERDATA, "/dev/block/mmcblk0p24" });
    device->setBootBlockDevs({ QCOM_BOOT, "/dev/block/mmcblk0p18" });
    device->setRecoveryBlockDevs({ QCOM_RECOVERY, "/dev/block/mmcblk0p19" });
    devices->push_back(device);
    
    // Huawei Y635 LTE
    device = new Device();
    device->setId("hwY635");
    device->setCodenames({ "hwY635", "Y635-L01", "Y635-L02", "Y635-L03", "Y635-L11",
        "Y635-L21", "hwy635" });
    device->setName("Huawei Y635 LTE");
    device->setBlockDevBaseDirs({ QCOM_BASE_DIR });
    device->setSystemBlockDevs({ QCOM_SYSTEM, "/dev/block/mmcblk0p23" });
    device->setCacheBlockDevs({ QCOM_CACHE, "/dev/block/mmcblk0p21" });
    device->setDataBlockDevs({ QCOM_USERDATA, "/dev/block/mmcblk0p24" });
    device->setBootBlockDevs({ QCOM_BOOT, "/dev/block/mmcblk0p18" });
    device->setRecoveryBlockDevs({ QCOM_RECOVERY, "/dev/block/mmcblk0p19" });
    devices->push_back(device);
    
    // Huawei Y550
    device = new Device();
    device->setId("Y550");
    device->setCodenames({ "y550", "Y550", "Y550-L01", "Y550-L02", "Y550-L03" });
    device->setName("Huawei Y550");
    device->setBlockDevBaseDirs({ QCOM_BASE_DIR });
    device->setSystemBlockDevs({ QCOM_SYSTEM, "/dev/block/mmcblk0p23" });
    device->setCacheBlockDevs({ QCOM_CACHE, "/dev/block/mmcblk0p21" });
    device->setDataBlockDevs({ QCOM_USERDATA, "/dev/block/mmcblk0p24" });
    device->setBootBlockDevs({ QCOM_BOOT, "/dev/block/mmcblk0p18" });
    device->setRecoveryBlockDevs({ QCOM_RECOVERY, "/dev/block/mmcblk0p19" });
    devices->push_back(device);
}

}
