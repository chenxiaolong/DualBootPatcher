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
}

}
