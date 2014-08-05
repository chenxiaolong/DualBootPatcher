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

from multiboot.autopatchers.cyanogenmod import DalvikCachePatcher
from multiboot.autopatchers.jflte import GoogleEditionPatcher
from multiboot.autopatchers.standard import StandardPatcher
import multiboot.autopatchers as autopatchers
import multiboot.config as config
import multiboot.fileio as fileio

import imp
import os
import zipfile


def insert_line(index, line, lines):
    lines.insert(index, line + '\n')
    return 1


def insert_partition_info(f, config, target_path_only=False):
    magic = '# PATCHER REPLACE ME - DO NOT REMOVE\n'
    lines = fileio.all_lines(f)

    i = 0
    while i < len(lines):
        if magic in lines[i]:
            del lines[i]

            if not target_path_only:
                i += insert_line(i, 'KERNEL_NAME=\"%s\"'
                                    % config.kernel, lines)

                i += insert_line(i, 'DEV_SYSTEM=\"%s\"'
                                    % config.dev_system, lines)
                i += insert_line(i, 'DEV_CACHE=\"%s\"'
                                    % config.dev_cache, lines)
                i += insert_line(i, 'DEV_DATA=\"%s\"'
                                    % config.dev_data, lines)

                i += insert_line(i, 'TARGET_SYSTEM_PARTITION=\"%s\"'
                                    % config.target_system_partition, lines)
                i += insert_line(i, 'TARGET_CACHE_PARTITION=\"%s\"'
                                    % config.target_cache_partition, lines)
                i += insert_line(i, 'TARGET_DATA_PARTITION=\"%s\"'
                                    % config.target_data_partition, lines)

            i += insert_line(i, 'TARGET_SYSTEM=\"%s\"'
                                % config.target_system, lines)
            i += insert_line(i, 'TARGET_CACHE=\"%s\"'
                                % config.target_cache, lines)
            i += insert_line(i, 'TARGET_DATA=\"%s\"'
                                % config.target_data, lines)

            break

        i += 1

    fileio.write_lines(f, lines)


class AutoPatcherPreset:
    def __init__(self):
        self.name = None
        self.autopatchers = None


def get_auto_patchers():
    aps = list()

    ap = AutoPatcherPreset()
    ap.name = 'Standard'
    ap.autopatchers = [StandardPatcher]
    aps.append(ap)

    ap = AutoPatcherPreset()
    ap.name = 'Standard + Google Edition Patcher'
    ap.autopatchers = [StandardPatcher, GoogleEditionPatcher]
    aps.append(ap)

    ap = AutoPatcherPreset()
    ap.name = 'Standard + /data/dalvik-cache Patcher'
    ap.autopatchers = [StandardPatcher, DalvikCachePatcher]
    aps.append(ap)

    return aps
