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

from collections import OrderedDict

import os
import sys
import yaml


# http://stackoverflow.com/a/21912744/1064977
def ordered_load(stream):
    class OrderedLoader(yaml.Loader):
        pass

    OrderedLoader.add_constructor(
        yaml.resolver.BaseResolver.DEFAULT_MAPPING_TAG,
        lambda loader, node: OrderedDict(loader.construct_pairs(node)))

    return yaml.load(stream, OrderedLoader)


with open(os.path.join(OS.rootdir, 'patcher.yaml'), 'r') as f:
    config = ordered_load(f)


def list_devices():
    return [x for x in config['devices']]


def has(*args):
    c = config
    for arg in args:
        if arg not in c:
            return False

        c = c[arg]

    return True


def get_version():
    return config['patcher']['version']


def get_selinux(device):
    if has('devices', device, 'selinux'):
        value = config['devices'][device]['selinux']
        if value and value != 'unchanged':
            return value

    return None


def get_partition(device, partition):
    if has('devices', device, 'partitions', partition):
        return config['devices'][device]['partitions'][partition]
    else:
        return None
