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

from multiboot.autopatchers.base import BasePatcher
import multiboot.fileio as fileio
import multiboot.patch as patch
import os


class PatchFilePatcher(BasePatcher):
    def __init__(self, **kwargs):
        super(PatchFilePatcher, self).__init__(**kwargs)

        self.patchfile = kwargs['patchfile']
        self.files_list = patch.files_in_patch(self.patchfile)

    def patch(self, directory, bootimg=None, device_check=True,
              partition_config=None, device=None):
        ret = patch.apply_patch(self.patchfile, directory)

        if not ret:
            self.error_msg = patch.error_msg
            return False
