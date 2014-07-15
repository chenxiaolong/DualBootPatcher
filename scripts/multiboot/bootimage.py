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

import multiboot.config as config
import multiboot.cmd as cmd
import multiboot.debug as debug
import multiboot.fileio as fileio
import multiboot.operatingsystem as OS
import multiboot.standalone.mkbootimg as mkbootimg
import multiboot.standalone.unlokibootimg as unlokibootimg
import multiboot.standalone.unpackbootimg as unpackbootimg

import os

error_msg = None


class BootImageInfo:
    def __init__(self):
        self.base = None
        self.cmdline = None
        self.pagesize = None

        # Offset
        self.ramdisk_offset = None
        self.second_offset = None
        self.tags_offset = None

        # Paths
        self.kernel = None
        self.ramdisk = None
        self.second = None
        self.dt = None


def create(info, output_file):
    global output, error, error_msg

    try:
        mkbootimg.build(
            output_file,
            board=None,
            base=info.base,
            cmdline=info.cmdline,
            page_size=info.pagesize,
            kernel_offset=None,
            ramdisk_offset=info.ramdisk_offset,
            second_offset=info.second_offset,
            tags_offset=info.tags_offset,
            kernel=info.kernel,
            ramdisk=info.ramdisk,
            second=info.second,
            dt=info.dt
        )
        return True
    except Exception as e:
        error_msg = 'Failed to create boot image: ' + str(e)
        return False


def extract(boot_image, output_dir, device=None):
    global output, error, error_msg

    try:
        if not OS.is_android() and not debug.is_debug():
            unlokibootimg.show_output = False
            unpackbootimg.show_output = False

        if unlokibootimg.is_loki(boot_image):
            unlokibootimg.extract(boot_image, output_dir)
        else:
            unpackbootimg.extract(boot_image, output_dir)
    except Exception as e:
        error_msg = 'Failed to extract boot image'
        return None

    info = BootImageInfo()

    prefix = os.path.join(output_dir, os.path.split(boot_image)[1])
    info.base = int(fileio.first_line(prefix + '-base'), 16)
    info.cmdline = fileio.first_line(prefix + '-cmdline')
    info.pagesize = int(fileio.first_line(prefix + '-pagesize'))
    os.remove(prefix + '-base')
    os.remove(prefix + '-cmdline')
    os.remove(prefix + '-pagesize')

    if os.path.exists(prefix + '-ramdisk_offset'):
        info.ramdisk_offset = \
            int(fileio.first_line(prefix + '-ramdisk_offset'), 16)
        os.remove(prefix + '-ramdisk_offset')
    elif device and config.has('devices', device, 'ramdisk'):
        # We need this for Loki'd boot images
        ramdiskopts = config.config['devices'][device]['ramdisk']
        info.ramdisk_offset = ramdiskopts['offset']

    if os.path.exists(prefix + '-second_offset'):
        info.second_offset = \
            int(fileio.first_line(prefix + '-second_offset'), 16)
        os.remove(prefix + '-second_offset')

    if os.path.exists(prefix + '-tags_offset'):
        info.tags_offset = \
            int(fileio.first_line(prefix + '-tags_offset'), 16)
        os.remove(prefix + '-tags_offset')

    info.kernel = prefix + '-zImage'
    if os.path.exists(prefix + '-ramdisk.gz'):
        info.ramdisk = prefix + '-ramdisk.gz'
    elif os.path.exists(prefix + '-ramdisk.lz4'):
        info.ramdisk = prefix + '-ramdisk.lz4'
    if os.path.exists(prefix + '-second'):
        info.second = prefix + '-second'
    if os.path.exists(prefix + '-dt'):
        info.dt = prefix + '-dt'

    return info
