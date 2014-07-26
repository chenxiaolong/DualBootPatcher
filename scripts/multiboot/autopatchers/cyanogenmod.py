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
import multiboot.fileio as fileio
import os


class DalvikCachePatcher:
    @staticmethod
    def store_in_data(directory, bootimg=None, device_check=True,
                      partition_config=None, device=None):
        lines = fileio.all_lines(os.path.join(directory, 'system/build.prop'))

        i = 0
        while i < len(lines):
            if 'dalvik.vm.dexopt-data-only' in lines[i]:
                del lines[i]

            else:
                i += 1

        fileio.write_lines(os.path.join(directory, 'system/build.prop'), lines)

    @staticmethod
    def files_for_store_in_data():
        return ['system/build.prop']


def get_auto_patchers():
    autopatchers = list()

    dalvikcache = autopatcher.AutoPatcher()
    dalvikcache.name = 'Standard + Store dalvik-cache in /data'
    dalvikcache.patcher = [autopatcher.auto_patch,
                             DalvikCachePatcher.store_in_data]
    dalvikcache.extractor = [autopatcher.files_to_auto_patch,
                               DalvikCachePatcher.files_for_store_in_data]

    autopatchers.append(dalvikcache)

    return autopatchers
