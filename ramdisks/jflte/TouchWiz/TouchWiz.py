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

version = None


def modify_init_rc(cpiofile):
    cpioentry = cpiofile.get_file('init.rc')
    lines = fileio.bytes_to_lines(cpioentry.content)
    buf = bytes()

    in_mediaserver = False

    for line in lines:
        if re.search(r"^.*setprop.*selinux.reload_policy.*$", line):
            buf += fileio.encode(re.sub(r"^", "#", line))

        elif 'check_icd' in line:
            buf += fileio.encode(re.sub(r"^", "#", line))

        elif version == 'kk44' and \
                line.startswith('service'):
            in_mediaserver = '/system/bin/mediaserver' in line
            buf += fileio.encode(line)

        elif in_mediaserver and re.search(r'^\s*user', line):
            buf += fileio.encode(fileio.whitespace(line) + 'user root\n')

        else:
            buf += fileio.encode(line)

    cpioentry.content = buf


def modify_init_target_rc(cpiofile):
    cpioentry = cpiofile.get_file('init.target.rc')
    lines = fileio.bytes_to_lines(cpioentry.content)
    buf = bytes()

    in_qcam = False

    for line in lines:
        if version == 'kk44' and \
                re.search(r"^on\s+fs_selinux\s*$", line):
            buf += fileio.encode(line)
            buf += fileio.encode("    mount_all fstab.qcom\n")
            buf += fileio.encode("    " + common.EXEC_MOUNT)

        elif re.search(r"^.*setprop.*selinux.reload_policy.*$", line):
            buf += fileio.encode(re.sub(r"^", "#", line))

        elif version == 'kk44' and \
                line.startswith('service'):
            in_qcam = 'qcamerasvr' in line
            buf += fileio.encode(line)

        # This is not exactly safe, but it's the best we can do. TouchWiz is
        # doing some funny business where a process running under a certain
        # user or group (confirmed by /proc/<pid>/status) cannot access files
        # by that group. Not only that, mm-qcamera-daemon doesn't work if the
        # process has multiple groups and root is not the primary group. Oh
        # well, I'm done debugging proprietary binaries.
        elif in_qcam and re.search(r'^\s*user', line):
            buf += fileio.encode(fileio.whitespace(line) + 'user root\n')

        elif in_qcam and re.search(r'^\s*group', line):
            buf += fileio.encode(fileio.whitespace(line) + 'group root\n')

        else:
            buf += fileio.encode(line)

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


def modify_ueventd_rc(cpiofile):
    if version != 'kk44':
        return

    cpioentry = cpiofile.get_file('ueventd.rc')
    lines = fileio.bytes_to_lines(cpioentry.content)
    buf = bytes()

    for line in lines:
        if '/dev/snd/*' in line:
            buf += fileio.encode(line.replace('0660', '0666'))

        else:
            buf += fileio.encode(line)

    cpioentry.content = buf


def modify_ueventd_qcom_rc(cpiofile):
    if version != 'kk44':
        return

    cpioentry = cpiofile.get_file('ueventd.qcom.rc')
    lines = fileio.bytes_to_lines(cpioentry.content)
    buf = bytes()

    for line in lines:
        # More funny business: even with mm-qcamera-daemon running as root,
        # the daemon and the default camera application are unable access the
        # camera hardware unless it's writable to everyone. Baffles me...
        #
        # More, more funny business: chmod'ing the /dev/video* devices to
        # anything while mm-qcamera-daemon is running causes a kernel panic.
        # **Wonderful** /s
        if '/dev/video*' in line:
            buf += fileio.encode(line.replace('0660', '0666'))

        elif '/dev/media*' in line:
            buf += fileio.encode(line.replace('0660', '0666'))

        elif '/dev/v4l-subdev*' in line:
            buf += fileio.encode(line.replace('0660', '0666'))

        elif '/dev/msm_camera/*' in line:
            buf += fileio.encode(line.replace('0660', '0666'))

        # Bah... even with the daemon running as root, it can't access the
        # hardware encoder
        elif '/dev/msm_vidc_enc' in line:
            buf += fileio.encode(line.replace('0660', '0666'))

        else:
            buf += fileio.encode(line)

    buf += fileio.encode('/dev/ion 0666 system system\n')

    cpioentry.content = buf


def patch_ramdisk(cpiofile, partition_config):
    global version

    if cpiofile.get_file('MSM8960_lpm.rc') is not None:
        version = 'jb43'
    else:
        version = 'kk44'

    qcom.modify_init_rc(cpiofile)
    modify_init_rc(cpiofile)
    qcom.modify_init_qcom_rc(cpiofile)
    qcom.modify_fstab(cpiofile, partition_config)
    qcom.modify_init_target_rc(cpiofile)
    modify_init_target_rc(cpiofile)
    modify_MSM8960_lpm_rc(cpiofile)
    modify_ueventd_rc(cpiofile)
    modify_ueventd_qcom_rc(cpiofile)

    # Samsung's init binary is pretty screwed up
    if version == 'kk44':
        newinit = os.path.join(OS.ramdiskdir, 'init', 'jflte', 'tw44-init')
        cpiofile.add_file(newinit, name='init', perms=0o755)

        newinit = os.path.join(OS.ramdiskdir, 'init', 'jflte', 'tw44-adbd')
        cpiofile.add_file(newinit, name='sbin/adbd', perms=0o755)

        mountscript = os.path.join(OS.ramdiskdir,
                                   'jflte', 'TouchWiz', 'mount.modem.sh')
        cpiofile.add_file(mountscript, name='init.additional.sh', perms=0o755)
