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

import multiboot.operatingsystem as OS

import os
import sys

if sys.hexversion < 0x03000000:
    import ConfigParser
    config = ConfigParser.ConfigParser()
    py3 = False
else:
    import configparser
    config = configparser.ConfigParser()
    py3 = True

config.read(os.path.join(OS.rootdir, 'patcher.conf'))


def get(section, option):
    if py3:
        return config[section][option]
    else:
        return config.get(section, option)


def has(section, option):
    if py3:
        return option in config[section]
    else:
        return config.has_option(section, option)


def list_devices():
    sections = config.sections()
    sections.remove('Defaults')
    return sections


def get_selinux(device):
    if has(device, 'selinux'):
        value = get(device, 'selinux')
        if value and value == 'unchanged':
            return None
        else:
            return value
    else:
        return None


def get_ramdisk_offset(device):
    if has(device, 'ramdisk_offset'):
        return get(device, 'ramdisk_offset')
    else:
        return None


def get_partition(device, partition):
    if has(device, 'partition.' + partition):
        return get(device, 'partition.' + partition)
    else:
        return None
