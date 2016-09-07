/*
 * Copyright (C) 2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "devices/google.h"

#include "devices/paths.h"

namespace mbp
{

void addGoogleDevices(std::vector<Device *> *devices)
{
    Device *device;

    // Google Android One
    device = new Device();
    device->setId("sprout");
    device->setCodenames({ "sprout", "sprout_b", "sprout4", "sprout8"});
    device->setName("Google Android One");
    device->setBlockDevBaseDirs({ MTK_BASE_DIR });
    device->setSystemBlockDevs({ MTK_SYSTEM, "/dev/block/mmcblk0p14" });
    device->setCacheBlockDevs({ MTK_CACHE, "/dev/block/mmcblk0p15" });
    device->setDataBlockDevs({ MTK_USERDATA, "/dev/block/mmcblk0p16" });
    device->setBootBlockDevs({ MTK_BOOT, "/dev/block/mmcblk0p7" });
    device->setRecoveryBlockDevs({ MTK_RECOVERY, "/dev/block/mmcblk0p8" });
    devices->push_back(device);
    
    // Infinix X510 (Android One)
        device = new Device();
        device->setId("d5110");
        device->setCodenames({ "d5110_infinix_sprout", "d5110_infinix", "Infinix_X510_sprout", "Infinix_X510", "X510"});
        device->setName("Hot 2/Android One");
        device->setBlockDevBaseDirs({ MTK_BASE_DIR });
        device->setSystemBlockDevs({ MTK_SYSTEM, "/dev/block/mmcblk0p23" });
        device->setCacheBlockDevs({ MTK_CACHE, "/dev/block/mmcblk0p24" });
        device->setDataBlockDevs({ MTK_USERDATA, "/dev/block/mmcblk0p25" });
        device->setBootBlockDevs({ MTK_BOOT, "/dev/block/mmcblk0p7" });
        device->setRecoveryBlockDevs({ MTK_RECOVERY, "/dev/block/mmcblk0p8" });
        devices->push_back(device);
        
    // Infinix X510 (Infinix ROM partition scheme)
        device = new Device();
        device->setId("d5110_infinix");
        device->setCodenames({ "d5110_infinix_sprout", "d5110_infinix", "Infinix_X510_sprout", "Infinix_X510", "X510"});
        device->setName("Hot 2/Infinix ROM");
        device->setBlockDevBaseDirs({ MTK_BASE_DIR });
        device->setSystemBlockDevs({ MTK_SYSTEM, "/dev/block/mmcblk0p18" });
        device->setCacheBlockDevs({ MTK_CACHE, "/dev/block/mmcblk0p19" });
        device->setDataBlockDevs({ MTK_USERDATA, "/dev/block/mmcblk0p20" });
        device->setBootBlockDevs({ MTK_BOOT, "/dev/block/mmcblk0p7" });
        device->setRecoveryBlockDevs({ MTK_RECOVERY, "/dev/block/mmcblk0p8" });
        devices->push_back(device);
}

}
