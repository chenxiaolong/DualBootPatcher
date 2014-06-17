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
import multiboot.operatingsystem as OS
import ramdisks.common.common as common
import ramdisks.common.qcom as qcom

import os
import re

# jb43, kk44
version = None


def modify_init_rc(cpiofile):
    cpioentry = cpiofile.get_file('init.rc')
    lines = fileio.bytes_to_lines(cpioentry.content)
    buf = bytes()

    previous_line = ""

    for line in lines:
        if version == 'kk44' \
                and re.search(r"mount.*/system", line) \
                and re.search(r"on\s+charger", previous_line):
            buf += fileio.encode("    mount_all fstab.jgedlte\n")
            buf += fileio.encode("    " + common.EXEC_MOUNT)

        else:
            buf += fileio.encode(line)

        previous_line = line

    cpioentry.content = buf


def modify_MSM8960_lpm_rc(cpiofile):
    if version == 'kk44':
        return

    cpioentry = cpiofile.get_file('MSM8960_lpm.rc')
    lines = fileio.bytes_to_lines(cpioentry.content)
    buf = bytes()

    for line in lines:
        if re.search(r"^\s+mount.*/cache.*$", line):
            buf += fileio.encode(re.sub(r"^", "#", line))

        else:
            buf += fileio.encode(line)

    cpioentry.content = buf


def patch_ramdisk(cpiofile, partition_config):
    global version

    if cpiofile.get_file('MSM8960_lpm.rc') is not None:
        version = 'jb43'
    else:
        version = 'kk44'

    qcom.modify_init_rc(cpiofile)
    modify_init_rc(cpiofile)
    qcom.modify_init_qcom_rc(cpiofile, additional=['init.jgedlte.rc'])
    qcom.modify_fstab(cpiofile, partition_config)
    qcom.modify_init_target_rc(cpiofile)
    modify_MSM8960_lpm_rc(cpiofile)

    # Samsung's init binary is pretty screwed up
    if version == 'kk44':
        newinit = os.path.join(OS.ramdiskdir, 'init', 'init-kk44')
        cpiofile.add_file(newinit, name='init', perms=0o755)
