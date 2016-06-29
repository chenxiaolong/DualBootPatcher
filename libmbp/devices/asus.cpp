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

#include "devices/asus.h"

#include "devices/paths.h"

namespace mbp
{

void addAsusDevices(std::vector<Device *> *devices)
{
    Device *device;

    // ASUS ZenFone 2 [ZE551ML/ZE550ML]
    device = new Device();
    device->setId("Z00A");
    device->setCodenames({ "Z00A", "Z008" });
    device->setName("ASUS ZenFone 2");
    device->setArchitecture(ARCH_X86);
    device->setBlockDevBaseDirs({ INTEL_PCI_BASE_DIR, BLOCK_BASE_DIR });
    device->setSystemBlockDevs({ INTEL_PCI_SYSTEM_2, BLOCK_SYSTEM,
                                 "/dev/block/mmcblk0p18" });
    device->setCacheBlockDevs({ INTEL_PCI_CACHE_2, BLOCK_CACHE,
                                "/dev/block/mmcblk0p15" });
    device->setDataBlockDevs({ INTEL_PCI_DATA_2, BLOCK_DATA,
                               "/dev/block/mmcblk0p19" });
    device->setBootBlockDevs({ INTEL_PCI_BOOT_2, BLOCK_BOOT,
                               "/dev/block/mmcblk0p1" });
    device->setRecoveryBlockDevs({ INTEL_PCI_RECOVERY_2, BLOCK_RECOVERY,
                                   "/dev/block/mmcblk0p2" });
    devices->push_back(device);

}

}
