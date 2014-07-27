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
import multiboot.config as config
import multiboot.fileio as fileio

import os
import re


UPDATER_SCRIPT = 'META-INF/com/google/android/updater-script'
MOUNT = 'run_program("/tmp/dualboot.sh", "mount-%s");'
UNMOUNT = 'run_program("/tmp/dualboot.sh", "unmount-%s");'
FORMAT = 'run_program("/tmp/dualboot.sh", "format-%s");'


def insert_line(index, line, lines):
    lines.insert(index, line + '\n')
    return 1


class StandardPatcher(BasePatcher):
    def __init__(self, **kwargs):
        super(StandardPatcher, self).__init__(**kwargs)

        self.files_list = [UPDATER_SCRIPT]

    def patch(self, directory, file_info, bootimages=None):
        lines = fileio.all_lines(os.path.join(directory, UPDATER_SCRIPT))

        StandardPatcher.insert_dual_boot_sh(lines)
        StandardPatcher.replace_mount_lines(file_info.device, lines)
        StandardPatcher.replace_unmount_lines(file_info.device, lines)
        StandardPatcher.replace_format_lines(file_info.device, lines)

        if bootimages:
            for img in bootimages:
                StandardPatcher.insert_write_kernel(img, lines)

        # Too many ROMs don't unmount partitions after installation
        StandardPatcher.insert_unmount_everything(len(lines), lines)

        # Remove device checks
        if not file_info.patchinfo.device_check:
            StandardPatcher.remove_device_checks(lines)

        fileio.write_lines(os.path.join(directory, UPDATER_SCRIPT), lines)

    @staticmethod
    def insert_dual_boot_sh(lines):
        i = 0
        i += insert_line(i, 'package_extract_file("dualboot.sh", "/tmp/dualboot.sh");', lines)
        i += insert_line(i, 'set_perm(0, 0, 0777, "/tmp/dualboot.sh");', lines)
        # TODO: Word this better for sure
        i += insert_line(i, 'ui_print("NOT INSTALLING AS PRIMARY");', lines)
        i += StandardPatcher.insert_unmount_everything(i, lines)
        return i

    @staticmethod
    def replace_mount_lines(device, lines):
        psystem = config.get_partition(device, 'system')
        pcache = config.get_partition(device, 'cache')
        pdata = config.get_partition(device, 'data')

        i = 0
        while i < len(lines):
            if re.search(r"^\s*mount\s*\(.*$", lines[i]) or \
                    re.search(r"^\s*run_program\s*\(\s*\"[^\"]*busybox\"\s*,\s*\"mount\".*$", lines[i]):
                # Mount /system as dual boot
                if 'system' in lines[i] or (psystem and psystem in lines[i]):
                    del lines[i]
                    i += StandardPatcher.insert_mount_system(i, lines)

                # Mount /cache as dual boot
                elif 'cache' in lines[i] or (pcache and pcache in lines[i]):
                    del lines[i]
                    i += StandardPatcher.insert_mount_cache(i, lines)

                # Mount /data as dual boot
                elif '"/data' in lines[i] or 'userdata' in lines[i] or \
                        (pdata and pdata in lines[i]):
                    del lines[i]
                    i += StandardPatcher.insert_mount_data(i, lines)

                else:
                    i += 1

            else:
                i += 1

    @staticmethod
    def replace_unmount_lines(device, lines):
        psystem = config.get_partition(device, 'system')
        pcache = config.get_partition(device, 'cache')
        pdata = config.get_partition(device, 'data')

        i = 0
        while i < len(lines):
            if re.search(r"^\s*unmount\s*\(.*$", lines[i]) or \
                    re.search(r"^\s*run_program\s*\(\s*\"[^\"]*busybox\"\s*,\s*\"umount\".*$", lines[i]):
                # Mount /system as dual boot
                if 'system' in lines[i] or (psystem and psystem in lines[i]):
                    del lines[i]
                    i += StandardPatcher.insert_unmount_system(i, lines)

                # Mount /cache as dual boot
                elif 'cache' in lines[i] or (pcache and pcache in lines[i]):
                    del lines[i]
                    i += StandardPatcher.insert_unmount_cache(i, lines)

                # Mount /data as dual boot
                elif '"/data' in lines[i] or 'userdata' in lines[i] or \
                        (pdata and pdata in lines[i]):
                    del lines[i]
                    i += StandardPatcher.insert_unmount_data(i, lines)

                else:
                    i += 1

            else:
                i += 1

    @staticmethod
    def replace_format_lines(device, lines):
        psystem = config.get_partition(device, 'system')
        pcache = config.get_partition(device, 'cache')
        pdata = config.get_partition(device, 'data')

        i = 0
        while i < len(lines):
            if re.search(r"^\s*format\s*\(.*$", lines[i]):
                # Mount /system as dual boot
                if 'system' in lines[i] or (psystem and psystem in lines[i]):
                    del lines[i]
                    i += StandardPatcher.insert_format_system(i, lines)

                # Mount /cache as dual boot
                elif 'cache' in lines[i] or (pcache and pcache in lines[i]):
                    del lines[i]
                    i += StandardPatcher.insert_format_cache(i, lines)

                # Mount /data as dual boot
                elif 'userdata' in lines[i] or (pdata and pdata in lines[i]):
                    del lines[i]
                    i += StandardPatcher.insert_format_data(i, lines)

                else:
                    i += 1

            elif re.search(r'delete_recursive\s*\([^\)]*"/system"', lines[i]):
                del lines[i]
                i += StandardPatcher.insert_format_system(i, lines)

            elif re.search(r'delete_recursive\s*\([^\)]*"/cache"', lines[i]):
                del lines[i]
                i += StandardPatcher.insert_format_cache(i, lines)

            else:
                i += 1

    @staticmethod
    def insert_write_kernel(bootimg, lines):
        set_kernel_line = 'run_program("/tmp/dualboot.sh", "set-multi-kernel");'

        # Look for last line containing the boot image string and insert after that
        # Some AOSP ROMs use 'loki.sh' now
        # Ktoonsez's updater-scripts use flash_kernel.sh now
        for search in ['loki.sh', 'flash_kernel.sh', bootimg]:
            i = len(lines) - 1
            while i > 0:
                if search in lines[i]:
                    # Statements can be on multiple lines, so insert after a
                    # semicolon is found
                    while i < len(lines):
                        if ';' in lines[i]:
                            insert_line(i + 1, set_kernel_line, lines)
                            return

                        else:
                            i += 1

                    break
                i -= 1

    @staticmethod
    def insert_mount_system(index, lines):
        return insert_line(index, MOUNT % 'system', lines)

    @staticmethod
    def insert_mount_cache(index, lines):
        return insert_line(index, MOUNT % 'cache', lines)

    @staticmethod
    def insert_mount_data(index, lines):
        return insert_line(index, MOUNT % 'data', lines)

    @staticmethod
    def insert_unmount_system(index, lines):
        return insert_line(index, UNMOUNT % 'system', lines)

    @staticmethod
    def insert_unmount_cache(index, lines):
        return insert_line(index, UNMOUNT % 'cache', lines)

    @staticmethod
    def insert_unmount_data(index, lines):
        return insert_line(index, UNMOUNT % 'data', lines)

    @staticmethod
    def insert_unmount_everything(index, lines):
        return insert_line(index, UNMOUNT % 'everything', lines)

    @staticmethod
    def insert_format_system(index, lines):
        return insert_line(index, FORMAT % 'system', lines)

    @staticmethod
    def insert_format_cache(index, lines):
        return insert_line(index, FORMAT % 'cache', lines)

    @staticmethod
    def insert_format_data(index, lines):
        return insert_line(index, FORMAT % 'data', lines)

    @staticmethod
    def remove_device_checks(lines):
        i = 0
        while i < len(lines):
            if re.search(r"^\s*assert\s*\(.*getprop\s*\(.*(ro.product.device|ro.build.product)", lines[i]):
                lines[i] = re.sub(r"^(\s*assert\s*\()", r"""\1"true" == "true" || """, lines[i])

            i += 1
