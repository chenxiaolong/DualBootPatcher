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
import multiboot.debug as debug
import multiboot.fileio as fileio
import multiboot.plugins as plugins
import multiboot.operatingsystem as OS
import ramdisks.common.common as common

import imp
import os
import re


def process_def(def_file, cpiofile, partition_config):
    debug.debug("Loading ramdisk definition %s" % def_file)

    for line in fileio.all_lines(def_file):
        if line.startswith("pyscript"):
            path = re.search(r"^pyscript\s*=\s*\"?(.*)\"?\s*$", line).group(1)

            debug.debug("Loading pyscript " + path)

            plugin = plugins.ramdisks[path]

            common.init(cpiofile, partition_config)
            plugin.patch_ramdisk(cpiofile, partition_config)

    return True


def get_all_ramdisks(device):
    directories = []
    if not device:
        devices = config.list_devices()
        for d in devices:
            directory = os.path.join(OS.ramdiskdir, d)
            if d not in directories and os.path.isdir(directory):
                directories.append(directory)
    else:
        directories.append(os.path.join(OS.ramdiskdir, device))

    ramdisks = []

    for directory in directories:
        for root, dirs, files in os.walk(directory):
            relative_path = os.path.relpath(root, OS.ramdiskdir)
            for f in files:
                if os.path.splitext(f)[1] == '.def':
                    ramdisks.append(os.path.join(relative_path, f))

    ramdisks.sort()
    return ramdisks


def get_all_inits():
    initdir = os.path.join(OS.ramdiskdir, 'init')
    inits = list()

    for root, dirs, files in os.walk(initdir):
        for f in files:
            inits.append(os.path.relpath(os.path.join(root, f), initdir))

    # Newest first
    inits.sort()
    inits.reverse()

    return inits
