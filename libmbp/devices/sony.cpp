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

#include "devices/sony.h"

#include "mbp/ramdiskpatchers/xperia.h"

#include "devices/paths.h"

namespace mbp
{

void addSonyDevices(std::vector<Device *> *devices)
{
    Device *device;

    // Sony Xperia Sola
    device = new Device();
    device->setId("pepper");
    device->setCodenames({ "pepper", "MT27a", "MT27i" });
    device->setName("Sony Xperia Sola");
    device->setFlags(Device::FLAG_HAS_COMBINED_BOOT_AND_RECOVERY);
    device->setRamdiskPatcher(XperiaDefaultRP::Id);
    device->setSystemBlockDevs({ "/dev/block/mmcblk0p10" });
    device->setCacheBlockDevs({ "/dev/block/mmcblk0p12" });
    device->setDataBlockDevs({ "/dev/block/mmcblk0p11" });
    device->setBootBlockDevs({ "/dev/block/mmcblk0p9" });
    devices->push_back(device);

    // Sony Xperia E1
    device = new Device();
    device->setId("falconss");
    device->setCodenames({ "falconss", "D2004", "D2005", "D2104", "D2105", "D2114" });
    device->setName("Sony Xperia E1");
    device->setFlags(Device::FLAG_HAS_COMBINED_BOOT_AND_RECOVERY);
    device->setRamdiskPatcher(XperiaDefaultRP::Id);
    device->setBlockDevBaseDirs({ QCOM_BASE_DIR });
    device->setSystemBlockDevs({ QCOM_SYSTEM, "/dev/block/mmcblk0p19" });
    device->setCacheBlockDevs({ QCOM_CACHE, "/dev/block/mmcblk0p20" });
    device->setDataBlockDevs({ QCOM_USERDATA, "/dev/block/mmcblk0p21" });
    device->setBootBlockDevs({ QCOM_BOOT, "/dev/block/mmcblk0p15" });
    device->setRecoveryBlockDevs({ QCOM_FOTA_RECOVERY, "/dev/block/mmcblk0p16" });
    devices->push_back(device);

    // Sony Xperia Z1
    device = new Device();
    device->setId("honami");
    device->setCodenames({ "honami", "C6903", "C6902", "C6906", "C6943" });
    device->setName("Sony Xperia Z1");
    device->setFlags(Device::FLAG_HAS_COMBINED_BOOT_AND_RECOVERY);
    device->setRamdiskPatcher(XperiaDefaultRP::Id);
    device->setBlockDevBaseDirs({ QCOM_BASE_DIR });
    device->setSystemBlockDevs({ QCOM_SYSTEM, "/dev/block/mmcblk0p23" });
    device->setCacheBlockDevs({ QCOM_CACHE, "/dev/block/mmcblk0p24" });
    device->setDataBlockDevs({ QCOM_USERDATA, "/dev/block/mmcblk0p25" });
    device->setBootBlockDevs({ QCOM_BOOT, "/dev/block/mmcblk0p14" });
    device->setRecoveryBlockDevs({ QCOM_FOTA_RECOVERY, "/dev/block/mmcblk0p16" });
    device->setExtraBlockDevs({ QCOM_ABOOT, QCOM_RPM, QCOM_SBL1, QCOM_TZ });
    devices->push_back(device);
}

}
