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

import multiboot.patchinfo as patchinfo

import copy
import imp
import os
import shutil


BOOT_IMAGE = 'img'
ZIP_FILE = 'zip'
UNSUPPORTED = 'UNSUPPORTED'


class FileInfo:
    def __init__(self):
        self.patchinfo = None
        self.device = None
        self._filename = None
        self.filetype = None
        self.partconfig = None

    def find_and_merge_patchinfo(self):
        info = patchinfo.find_patchinfo(self.filename, self.device)

        if not info:
            return False

        self.patchinfo = copy.copy(info)

        return True

    @property
    def filename(self):
        return self._filename

    @filename.setter
    def filename(self, value):
        extension = os.path.splitext(value)[1]
        if extension.lower() == '.zip':
            self.filetype = ZIP_FILE
        elif extension.lower() == '.img':
            self.filetype = BOOT_IMAGE
        elif extension.lower() == '.lok':
            self.filetype = BOOT_IMAGE
        else:
            self.filetype = UNSUPPORTED

        self._filename = value

    def get_new_filename(self):
        fileext = os.path.splitext(self._filename)
        base = fileext[0] + '_' + self.partconfig.id
        return base + fileext[1]

    def move_to_target(self, newfile, move=False, delete=True):
        targetfile = self.get_new_filename()

        if os.path.exists(targetfile):
            os.remove(targetfile)

        if move:
            shutil.move(newfile, targetfile)
        else:
            shutil.copyfile(newfile, targetfile)
            if delete:
                os.remove(newfile)

    def is_filetype_supported(self):
        return self.filetype != UNSUPPORTED
