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

import multiboot.fileio as fileio
import ramdisks.common.qcom as qcom


def modify_init_target_rc(cpiofile):
    cpioentry = cpiofile.get_file('init.target.rc')
    lines = fileio.bytes_to_lines(cpioentry.content)
    buf = bytes()

    for line in lines:
        if 'init.dualboot.mounting.sh' in line:
            buf += fileio.encode(line.replace('init.dualboot.mounting.sh',
                                              'init.multiboot.mounting.sh'))

        else:
            buf += fileio.encode(line)

    cpioentry.set_content(buf)


def patch_ramdisk(cpiofile, partition_config):
    # /raw-cache needs to always be mounted rw so OpenDelta can write to
    # /cache/recovery
    qcom.modify_fstab(cpiofile, partition_config,
                      force_cache_rw=True,
                      keep_mountpoints=True,
                      system_mountpoint='/raw-system',
                      cache_mountpoint='/raw-cache',
                      data_mountpoint='/raw-data')

    modify_init_target_rc(cpiofile)
