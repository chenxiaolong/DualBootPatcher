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


import multiboot.autopatcher as autopatcher
import multiboot.config as config
import multiboot.fileio as fileio
import multiboot.operatingsystem as OS
import multiboot.partitionconfigs as partitionconfigs

import imp
import functools
import os


class PatchInfo:
    """Patch information

    This contains information on how a ROM or kernel should be patched.
    """

    def __init__(self):
        # Name of the ROM/Kernel
        self.name = None

        # Function or regex for matching the file
        self._matches = None

        # Patcher or patch file
        self._patch = None

        # Files to extract (string, list, or function)
        self._extract = None

        # Ramdisk definition (.def file)
        self._ramdisk = None

        # Whether the file contains a boot image
        self.has_boot_image = True

        # Boot images (string, list, or function)
        self._bootimg = [autopatcher.autodetect_boot_images]

        # Whether device checks in updater-script should be kept
        self.device_check = True

        # Supported partition configurations
        self._configs = ['all']

        # Replace init binary with another binary
        self._patched_init = None

        # Callback for modifying variables when filename is set
        self._on_filename_set = None

    @property
    def matches(self):
        return self._matches

    @matches.setter
    def matches(self, value):
        if type(value) == str:
            self._matches = self._function_matches_regex(value)
        elif callable(value):
            self._matches = value
        else:
            raise ValueError('Invalid type')

    def _function_matches_regex(self, regex):
        return functools.partial(fileio.filename_matches, regex)

    @property
    def patch(self):
        return self._patch

    @patch.setter
    def patch(self, value):
        if type(value) == str or callable(value):
            temp = [value]
        elif type(value) == list:
            temp = value
        else:
            raise ValueError('Invalid type')

        self._patch = temp

    @property
    def extract(self):
        return self._extract

    @extract.setter
    def extract(self, value):
        if type(value) == str or callable(value):
            temp = [value]
        elif type(value) == list:
            temp = value
        else:
            raise ValueError('Invalid type')

        self._extract = temp

    @property
    def ramdisk(self):
        return self._ramdisk

    @ramdisk.setter
    def ramdisk(self, value):
        ramdiskfile = os.path.join(OS.ramdiskdir, value)
        if not os.path.exists(ramdiskfile):
            raise IOError('Ramdisk definition %s not found' % value)

        self._ramdisk = value

    @property
    def bootimg(self):
        if self.has_boot_image:
            return self._bootimg
        else:
            return None

    @bootimg.setter
    def bootimg(self, value):
        if type(value) == str or callable(value):
            temp = [value]
        elif type(value) == list:
            temp = value
        else:
            raise ValueError('Invalid type')

        self._bootimg = temp

    @property
    def configs(self):
        return self._configs

    @configs.setter
    def configs(self, value):
        if type(value) != list:
            raise ValueError('configs must be set to a list')

        for i in value:
            if i[0] == '!':
                partconfig_id = i[1:]
            else:
                partconfig_id = i

            if not self._is_valid_partconfig(partconfig_id):
                raise ValueError('%s is not a supported partition config'
                                 % partconfig_id)

        self._configs = value

    def _is_valid_partconfig(self, partconfig_id):
        if partconfig_id == 'all':
            return True

        for partconfig in partitionconfigs.get():
            if partconfig.id == partconfig_id:
                return True

        return False

    def is_partconfig_supported(self, partconfig):
        return ('all' in self.configs or partconfig.id in self.configs) \
            and ('!' + partconfig.id) not in self.configs

    @property
    def patched_init(self):
        return self._patched_init

    @patched_init.setter
    def patched_init(self, value):
        if not value:
            self._patched_init = None
            return

        initfile = os.path.join(OS.ramdiskdir, 'init', value)
        if not os.path.exists(initfile):
            raise IOError('Patched init binary %s not found' % value)

        self._patched_init = value

    @property
    def on_filename_set(self):
        return self._on_filename_set

    @on_filename_set.setter
    def on_filename_set(self, value):
        if not callable(value):
            return ValueError('on_filename_set must be set to a function')

        self._on_filename_set = value


def find_patchinfo(filename, device=None):
    """Find the matching PatchInfo object generated by a patchinfo script.

    The returned object should not be changed.
    """

    searchpath = ['Google_Apps', 'Other']

    if device:
        searchpath.append(device)

    for d in searchpath:
        for root, dirs, files in os.walk(os.path.join(OS.patchinfodir, d)):
            for f in files:
                if f.endswith('.py'):
                    # Dynamically load patchinfo script
                    plugin = imp.load_source(os.path.basename(f)[:-3],
                                             os.path.join(root, f))

                    patchinfo = plugin.patchinfo

                    if patchinfo.matches(filename):
                        if patchinfo.on_filename_set:
                            patchinfo.on_filename_set(patchinfo, filename)

                        return patchinfo

    return None


def get_all_patchinfos(device):
    """Get all the PatchInfo objects for a particular device.

    The returned objects should not be changed.
    """

    infos = list()

    searchpath = ['Google_Apps', 'Other']

    if device:
        searchpath.append(device)
    else:
        devices = config.list_devices()
        for d in devices:
            if os.path.isdir(os.path.join(OS.patchinfodir, d)):
                searchpath.append(d)

    for d in searchpath:
        for root, dirs, files in os.walk(os.path.join(OS.patchinfodir, d)):
            for f in files:
                if f.endswith('.py'):
                    relpath = os.path.relpath(os.path.join(root, f),
                                              OS.patchinfodir)
                    plugin = imp.load_source(os.path.basename(f)[:-3],
                                             os.path.join(root, f))

                    infos.append((relpath, plugin.patchinfo))

    infos.sort()
    return infos
