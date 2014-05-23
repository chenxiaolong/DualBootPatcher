# Copyright (C) 2014  Xiao-Long Chen <chenxiaolong@cxl.epac.to>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

from multiboot.partitionconfig import PartitionConfig

configs = []

################################################################

dual = PartitionConfig()

dual.name                    = "Dual Boot"
dual.description             = "Second ROM installed to /system/dual"
dual.kernel                  = "secondary"
dual.id                      = "dualboot"

dual.target_system           = "/raw-system/dual"
dual.target_cache            = "/raw-cache/dual"
dual.target_data             = "/raw-data/dual"

dual.target_system_partition = PartitionConfig.SYSTEM
dual.target_cache_partition  = PartitionConfig.CACHE
dual.target_data_partition   = PartitionConfig.DATA

configs.append(dual)

################################################################

multislot1 = PartitionConfig()

multislot1.name                    = "Multi Boot Slot 1"
multislot1.description             = "ROM installed to /cache/multi-slot-1/system"
multislot1.kernel                  = "multi-slot-1"
multislot1.id                      = "multislot1"

multislot1.target_system           = "/raw-cache/multi-slot-1/system"
multislot1.target_cache            = "/raw-system/multi-slot-1/cache"
multislot1.target_data             = "/raw-data/multi-slot-1"

multislot1.target_system_partition = PartitionConfig.CACHE
multislot1.target_cache_partition  = PartitionConfig.SYSTEM
multislot1.target_data_partition   = PartitionConfig.DATA

configs.append(multislot1)

################################################################

multislot2 = PartitionConfig()

multislot2.name                    = "Multi Boot Slot 2"
multislot2.description             = "ROM installed to /cache/multi-slot-2/system"
multislot2.kernel                  = "multi-slot-2"
multislot2.id                      = "multislot2"

multislot2.target_system           = "/raw-cache/multi-slot-2/system"
multislot2.target_cache            = "/raw-system/multi-slot-2/cache"
multislot2.target_data             = "/raw-data/multi-slot-2"

multislot2.target_system_partition = PartitionConfig.CACHE
multislot2.target_cache_partition  = PartitionConfig.SYSTEM
multislot2.target_data_partition   = PartitionConfig.DATA

configs.append(multislot2)

################################################################

multislot3 = PartitionConfig()

multislot3.name                    = "Multi Boot Slot 3"
multislot3.description             = "ROM installed to /cache/multi-slot-3/system"
multislot3.kernel                  = "multi-slot-3"
multislot3.id                      = "multislot3"

multislot3.target_system           = "/raw-cache/multi-slot-3/system"
multislot3.target_cache            = "/raw-system/multi-slot-3/cache"
multislot3.target_data             = "/raw-data/multi-slot-3"

multislot3.target_system_partition = PartitionConfig.CACHE
multislot3.target_cache_partition  = PartitionConfig.SYSTEM
multislot3.target_data_partition   = PartitionConfig.DATA

configs.append(multislot3)

################################################################


def get():
    return configs
