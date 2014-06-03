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
import ramdisks.common.common as common

import difflib
import re

SYSTEM_PART = '/dev/block/platform/msm_sdcc.1/by-name/system'
CACHE_PART = '/dev/block/platform/msm_sdcc.1/by-name/cache'
DATA_PART = '/dev/block/platform/msm_sdcc.1/by-name/userdata'
APNHLOS_PART = '/dev/block/platform/msm_sdcc.1/by-name/apnhlos'
MDM_PART = '/dev/block/platform/msm_sdcc.1/by-name/mdm'


def modify_init_rc(cpiofile):
    cpioentry = cpiofile.get_file('init.rc')
    lines = fileio.bytes_to_lines(cpioentry.content)
    buf = bytes()

    for line in lines:
        if re.search(r"mkdir /system(\s|$)", line):
            buf += fileio.encode(line)
            buf += fileio.encode(re.sub("/system", "/raw-system", line))

        elif re.search(r"mkdir /data(\s|$)", line):
            buf += fileio.encode(line)
            buf += fileio.encode(re.sub("/data", "/raw-data", line))

        elif re.search(r"mkdir /cache(\s|$)", line):
            buf += fileio.encode(line)
            buf += fileio.encode(re.sub("/cache", "/raw-cache", line))

        elif 'yaffs2' in line:
            buf += fileio.encode(re.sub(r"^", "#", line))

        else:
            buf += fileio.encode(line)

    cpioentry.set_content(buf)


def modify_init_qcom_rc(cpiofile, additional=None):
    files = ['init.qcom.rc']

    if additional:
        files.extend(additional)

    for f in files:
        cpioentry = cpiofile.get_file(f)
        lines = fileio.bytes_to_lines(cpioentry.content)
        buf = bytes()

        for line in lines:
            # Change /data/media to /raw-data/media
            if re.search(r"/data/media(\s|$)", line):
                buf += fileio.encode(
                    re.sub('/data/media', '/raw-data/media', line))

            else:
                buf += fileio.encode(line)

        cpioentry.set_content(buf)


def modify_fstab(cpiofile, partition_config):
    move_apnhlos_mount = False
    move_mdm_mount = False

    fstabs = list()
    for m in cpiofile.members:
        if m.name.startswith('fstab.'):
            fstabs.append(m.name)

    for fstab in fstabs:
        cpioentry = cpiofile.get_file(fstab)
        lines = fileio.bytes_to_lines(cpioentry.content)
        buf = bytes()

        mount_line = '%s%s %s %s %s %s\n'

        # For Android 4.2 ROMs
        has_cache_line = False

        # Find mount options and vold options for the system and cache
        # partitions
        system_mountargs = dict()
        system_voldargs = dict()
        cache_mountargs = dict()
        cache_voldargs = dict()

        for line in lines:
            m = re.search(common.FSTAB_REGEX, line)
            if not m:
                continue

            comment = m.group(1)
            blockdev = m.group(2)
            mountpoint = m.group(3)
            fstype = m.group(4)
            mountargs = m.group(5)
            voldargs = m.group(6)

            if comment is None:
                comment = ''

            if blockdev == SYSTEM_PART:
                system_mountargs[comment] = mountargs
                system_voldargs[comment] = voldargs

            elif blockdev == CACHE_PART:
                cache_mountargs[comment] = mountargs
                cache_voldargs[comment] = voldargs

        # If, for whatever reason, these aren't in the fstab, then choose some
        # sensible defaults
        if not system_mountargs:
            system_mountargs[''] = 'ro,barrier=1,errors=panic'
        if not system_voldargs:
            system_voldargs[''] = 'wait'
        if not cache_mountargs:
            cache_mountargs[''] = 'nosuid,nodev,barrier=1'
        if not cache_voldargs:
            cache_voldargs[''] = 'wait,check'

        for line in lines:
            m = re.search(common.FSTAB_REGEX, line)
            if m:
                comment = m.group(1)
                blockdev = m.group(2)
                mountpoint = m.group(3)
                fstype = m.group(4)
                mountargs = m.group(5)
                voldargs = m.group(6)

                if comment is None:
                    comment = ''

            if m and blockdev == SYSTEM_PART and mountpoint == '/system':
                mountpoint = '/raw-system'

                if '/raw-system' in partition_config.target_cache:
                    cache_comment = difflib.get_close_matches(
                        comment, cache_mountargs, 1, 0)[0]
                    mountargs = cache_mountargs[cache_comment]
                    voldargs = cache_voldargs[cache_comment]

                temp = mount_line % (comment, blockdev, mountpoint,
                                     fstype, mountargs, voldargs)
                buf += fileio.encode(temp)

            elif m and blockdev == CACHE_PART and mountpoint == '/cache':
                mountpoint = '/raw-cache'
                has_cache_line = True

                if '/raw-cache' in partition_config.target_system:
                    system_comment = difflib.get_close_matches(
                        comment, system_mountargs, 1, 0)[0]
                    mountargs = system_mountargs[system_comment]
                    voldargs = system_voldargs[system_comment]

                temp = mount_line % (comment, blockdev, mountpoint,
                                     fstype, mountargs, voldargs)
                buf += fileio.encode(temp)

            elif m and blockdev == DATA_PART and mountpoint == '/data':
                mountpoint = '/raw-data'

                if '/raw-data' in partition_config.target_system:
                    system_comment = difflib.get_close_matches(
                        comment, system_mountargs, 1, 0)[0]
                    mountargs = system_mountargs[system_comment]
                    voldargs = system_voldargs[system_comment]

                temp = mount_line % (comment, blockdev, mountpoint,
                                     fstype, mountargs, voldargs)
                buf += fileio.encode(temp)

            elif m and blockdev == APNHLOS_PART:
                move_apnhlos_mount = True
                continue

            elif m and blockdev == MDM_PART:
                move_mdm_mount = True
                continue

            else:
                buf += fileio.encode(line)

        if not has_cache_line:
            cache_line = '%s /raw-cache ext4 %s %s\n'

            if '/raw-cache' in partition_config.target_system:
                mountargs = system_mountargs['']
                voldargs = system_voldargs['']
            else:
                mountargs = cache_mountargs['']
                voldargs = cache_voldargs['']

            buf += fileio.encode(cache_line %
                                 (CACHE_PART, mountargs, voldargs))

        cpioentry.set_content(buf)

    return (move_apnhlos_mount, move_mdm_mount)


def modify_init_target_rc(cpiofile, insert_apnhlos=False,
                          insert_mdm=False):
    cpioentry = cpiofile.get_file('init.target.rc')
    lines = fileio.bytes_to_lines(cpioentry.content)
    buf = bytes()

    for line in lines:
        if re.search(r"^\s+wait\s+/dev/.*/cache.*$", line):
            buf += fileio.encode(re.sub(r"^", "#", line))

        elif re.search(r"^\s+check_fs\s+/dev/.*/cache.*$", line):
            buf += fileio.encode(re.sub(r"^", "#", line))

        elif re.search(r"^\s+mount\s+ext4\s+/dev/.*/cache.*$", line):
            buf += fileio.encode(re.sub(r"^", "#", line))

        elif re.search(r"^\s+mount_all\s+fstab\..*$", line):
            buf += fileio.encode(line)
            buf += fileio.encode(fileio.whitespace(line) + common.EXEC_MOUNT)

        elif re.search(r"^\s+setprop\s+ro.crypto.fuse_sdcard\s+true", line):
            buf += fileio.encode(line)

            voldargs = 'shortname=lower,uid=1000,gid=1000,dmask=227,fmask=337'

            if insert_apnhlos:
                buf += fileio.encode(fileio.whitespace(line) +
                                     "wait %s\n" % qcom.APNHLOS_PART)
                buf += fileio.encode(fileio.whitespace(line) +
                                     "mount vfat %s /firmware ro %s\n" %
                                     (qcom.APNHLOS_PART, voldargs))

            if insert_mdm:
                buf += fileio.encode(fileio.whitespace(line) +
                                     "wait %s\n" % qcom.MDM_PART)
                buf += fileio.encode(fileio.whitespace(line) +
                                     "mount vfat %s /firmware-mdm ro %s\n" %
                                     (qcom.MDM_PART, voldargs))

        else:
            buf += fileio.encode(line)

    cpioentry.set_content(buf)
