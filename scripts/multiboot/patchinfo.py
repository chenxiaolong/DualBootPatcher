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
import multiboot.fileio as fileio
import multiboot.operatingsystem as OS
import multiboot.partitionconfigs as partitionconfigs
import multiboot.plugins as plugins
import multiboot.standalone.unpackbootimg as unpackbootimg

import imp
import functools
import os
import zipfile


class PatchInfo(object):
    """Patch information

    This contains information on how a ROM or kernel should be patched.
    """

    def __init__(self):
        super(PatchInfo, self).__init__()

        # Name of the ROM/Kernel
        self.name = None

        # Function or regex for matching the file
        self._matches = None

        # List of AutoPatcher instances
        self._autopatchers = None

        # Ramdisk definition (.def file)
        self._ramdisk = None

        # Whether the file contains a boot image
        self.has_boot_image = True

        # Boot images (string, list, or function)
        self._bootimg = [autodetect_boot_images]

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
    def autopatchers(self):
        return self._autopatchers

    @autopatchers.setter
    def autopatchers(self, value):
        if type(value) != list:
            self._autopatchers = [value]
        else:
            self._autopatchers = value

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


def is_android_boot_image(data):
    upper = 512 + unpackbootimg.BOOT_MAGIC_SIZE

    if len(data) < upper:
        return False

    section = data[0:upper]
    return section.find(unpackbootimg.BOOT_MAGIC.encode('ASCII')) != -1


def autodetect_boot_images(filename):
    boot_images = list()

    with zipfile.ZipFile(filename, 'r') as z:
        for name in z.namelist():
            if name.lower().endswith('.img') or name.lower().endswith('.lok'):
                # Check to make sure it's actually a kernel image
                if is_android_boot_image(z.read(name)):
                    boot_images.append(name)

    if boot_images:
        return boot_images
    else:
        return None


def find_patchinfo(filename, device=None):
    """Find the matching PatchInfo object generated by a patchinfo script.

    The returned object should not be changed.
    """

    for d in plugins.patchinfos:
        if d in plugins.always_include or d == device:
            for info in plugins.patchinfos[d]:
                patchinfo = info[1]

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

    for d in plugins.patchinfos:
        if not device or (d in plugins.always_include or d == device):
            for info in plugins.patchinfos[d]:
                infos.append(info)

    infos.sort()
    return infos
