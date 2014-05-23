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

class PartitionConfig:
    # Location, filesystem, and mount point of the raw partitions
    dev_system = "/dev/block/platform/msm_sdcc.1/by-name/system::ext4::/raw-system"
    dev_cache = "/dev/block/platform/msm_sdcc.1/by-name/cache::ext4::/raw-cache"
    dev_data = "/dev/block/platform/msm_sdcc.1/by-name/userdata::ext4::/raw-data"

    SYSTEM = "$DEV_SYSTEM"
    CACHE = "$DEV_CACHE"
    DATA = "$DEV_DATA"

    def __init__(self):
        self.name = ""
        self.description = ""
        self.kernel = ""
        self.id = ""

        self.target_system = ""
        self.target_cache = ""
        self.target_data = ""

        self.target_system_partition = ""
        self.target_cache_partition = ""
        self.target_data_partition = ""
