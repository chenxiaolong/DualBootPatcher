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

from multiboot.autopatchers.patchfile import PatchFilePatcher
import multiboot.autopatcher as autopatcher
import multiboot.bootimage as bootimage
import multiboot.config as config
import multiboot.fileinfo as fileinfo
import multiboot.fileio as fileio
import multiboot.minicpio.minicpio as minicpio
import multiboot.operatingsystem as OS
import multiboot.ramdisk as ramdisk

import gzip
import os
import re
import shutil
import tempfile
import zipfile


UPDATER_SCRIPT = 'META-INF/com/google/android/updater-script'

FORMAT = 'run_program("/sbin/busybox", "find", "%s",' \
    ' "-maxdepth", "1",' \
    ' "!", "-name", "multi-slot-*",' \
    ' "!", "-name", "dual");'
FORMAT_SYSTEM = FORMAT % '/system'
FORMAT_CACHE = FORMAT % '/cache'

MOUNT = 'run_program("/sbin/busybox", "mount", "%s");'
UNMOUNT = 'run_program("/sbin/busybox", "umount", "%s");'

PERM_TOOL = 'backuppermtool.sh'
PERMS_BACKUP = 'run_program("/tmp/%s", "backup", "%%s");' % PERM_TOOL
PERMS_RESTORE = 'run_program("/tmp/%s", "restore", "%%s");' % PERM_TOOL
EXTRACT = 'package_extract_file("%s", "%s");'
MAKE_EXECUTABLE = 'set_perm(0, 0, 0777, "%s");'


class Patcher(object):
    @staticmethod
    def get_patcher_by_partconfig(file_info):
        if file_info.partconfig.id == 'primaryupgrade':
            return PrimaryUpgradePatcher(file_info)
        else:
            return MultibootPatcher(file_info)

    def __init__(self, file_info):
        super(Patcher, self).__init__()

        self.tasks = dict()
        self.file_info = file_info

        # Some sanity checks
        if not self.file_info.filename \
                or not os.path.exists(self.file_info.filename):
            raise Exception('%s does not exist' % self.file_info.filename)

        if not self.file_info.filetype:
            raise Exception('File type not set')

    def add_task(self, task, task_name):
        self.tasks[task] = task_name
        OS.ui.add_task(task_name)

    def start_patching(self):
        raise Exception('start_patching() must be overrided')


class MultibootPatcher(Patcher):
    def __init__(self, file_info):
        super(MultibootPatcher, self).__init__(file_info)

        # List of temporary directories created while patching
        self.tempdirs = list()

        if self.file_info.filetype == fileinfo.ZIP_FILE:
            self.add_task('EXTRACTING_ZIP', 'Extracting zip file')

        if self.file_info.filetype == fileinfo.BOOT_IMAGE \
                or self.file_info.patchinfo.has_boot_image:
            self.add_task('PATCHING_RAMDISK', 'Patching ramdisk')

        if self.file_info.filetype == fileinfo.ZIP_FILE:
            self.add_task('PATCHING_FILES', 'Patching files')
            self.add_task('COMPRESSING_ZIP_FILE', 'Compressing zip file')

    def start_patching(self):
        newfile = None

        try:
            if self.file_info.filetype == fileinfo.ZIP_FILE:
                newfile = self.patch_zip()

                if newfile is not None:
                    OS.ui.succeeded('Successfully patched zip file')

            elif self.file_info.filetype == fileinfo.BOOT_IMAGE:
                newfile = self.patch_boot_image()

                if newfile is not None:
                    OS.ui.succeeded('Successfully patched boot image')

            else:
                OS.ui.failed('Unsupported file extension')

        except Exception as e:
            OS.ui.failed(str(e))

        self.cleanup()

        return newfile

    def patch_boot_image(self, path=None):
        if not path:
            path = self.file_info.filename

        OS.ui.set_task(self.tasks['PATCHING_RAMDISK'])

        tempdir = tempfile.mkdtemp()
        self.tempdirs.append(tempdir)

        OS.ui.details('Unpacking boot image ...')

        bootimageinfo = bootimage.extract(path, tempdir,
                                          device=self.file_info.device)
        if not bootimageinfo:
            OS.ui.failed(bootimage.error_msg)
            return None

        selinux = config.get_selinux(self.file_info.device)
        if selinux is not None:
            bootimageinfo.cmdline += ' androidboot.selinux=' + selinux

        target_ramdisk = os.path.join(tempdir, 'ramdisk.cpio')
        target_kernel = os.path.join(tempdir, 'kernel.img')

        shutil.move(bootimageinfo.kernel, target_kernel)

        # Minicpio
        OS.ui.details('Loading ramdisk with minicpio ...')
        cf = minicpio.CpioFile()
        with gzip.open(bootimageinfo.ramdisk, 'rb') as f:
            cf.load(f.read())

        os.remove(bootimageinfo.ramdisk)

        # Patch ramdisk
        if not self.file_info.patchinfo.ramdisk:
            OS.ui.info('No ramdisk patch specified')
            return None

        OS.ui.details('Modifying ramdisk ...')

        try:
            ramdisk.process_def(
                os.path.join(OS.ramdiskdir, self.file_info.patchinfo.ramdisk),
                cf,
                self.file_info.partconfig
            )
        except Exception as e:
            OS.ui.failed('Failed to patch ramdisk: ' + str(e))
            return None

        # Copy init.multiboot.mounting.sh
        shutil.copy(
            os.path.join(OS.ramdiskdir, 'init.multiboot.mounting.sh'),
            tempdir
        )
        autopatcher.insert_partition_info(
            tempdir, 'init.multiboot.mounting.sh',
            self.file_info.partconfig, target_path_only=True
        )
        cf.add_file(
            os.path.join(tempdir, 'init.multiboot.mounting.sh'),
            name='init.multiboot.mounting.sh',
            perms=0o750
        )
        os.remove(os.path.join(tempdir, 'init.multiboot.mounting.sh'))

        # Copy busybox
        cf.add_file(
            os.path.join(OS.ramdiskdir, 'busybox-static'),
            name='sbin/busybox-static',
            perms=0o750
        )

        # Copy syncdaemon
        cf.add_file(
            os.path.join(OS.ramdiskdir, 'syncdaemon'),
            name='sbin/syncdaemon',
            perms=0o750
        )

        # Copy patched init if needed
        if self.file_info.patchinfo.patched_init is not None:
            cf.add_file(
                os.path.join(OS.ramdiskdir, 'init',
                             self.file_info.patchinfo.patched_init),
                name='init',
                perms=0o755
            )

        # Create gzip compressed ramdisk
        OS.ui.details('Creating gzip compressed ramdisk with minicpio ...')

        target_ramdisk = target_ramdisk + ".gz"

        with gzip.open(target_ramdisk, 'wb') as f_out:
            data = cf.create()

            if data is not None:
                f_out.write(data)
            else:
                OS.ui.failed('Failed to create gzip compressed ramdisk')
                return None

        OS.ui.details('Running mkbootimg ...')

        bootimageinfo.kernel = target_kernel
        bootimageinfo.ramdisk = target_ramdisk

        new_boot_image = tempfile.mkstemp()

        if not bootimage.create(bootimageinfo, new_boot_image[1]):
            OS.ui.failed('Failed to create boot image: ' + bootimage.error_msg)
            os.close(new_boot_image[0])
            os.remove(new_boot_image[1])
            return None

        os.remove(target_kernel)
        os.remove(target_ramdisk)

        os.close(new_boot_image[0])
        return new_boot_image[1]

    def patch_zip(self):
        if not OS.is_android():
            OS.ui.info('--- Please wait. This may take a while ---')

        ap_instances = []
        files_to_patch = []

        if self.file_info.patchinfo.autopatchers:
            for ap in self.file_info.patchinfo.autopatchers:
                if type(ap) == str:
                    # Special case for patch files
                    ap_instances.append(PatchFilePatcher(patchfile=ap))
#                elif issubclass(ap, BasePatcher):
                elif True:
                    ap_instances.append(ap())
                else:
                    raise ValueError('Invalid autopatcher type')

            for ap in ap_instances:
                for f in ap.files_list:
                    if f not in files_to_patch:
                        files_to_patch.append(f)

        tempdir = tempfile.mkdtemp()
        self.tempdirs.append(tempdir)

        OS.ui.set_task(self.tasks['EXTRACTING_ZIP'])

        OS.ui.details('Loading zip file ...')
        z = zipfile.ZipFile(self.file_info.filename, 'r')

        for f in files_to_patch:
            OS.ui.details("Extracting file to be patched: %s" % f)
            try:
                z.extract(f, path=tempdir)
            except:
                OS.ui.failed('Failed to extract file: %s' % f)
                return None

        bootimages = list()

        if self.file_info.patchinfo.has_boot_image:
            for i in self.file_info.patchinfo.bootimg:
                if callable(i):
                    output = i(self.file_info.filename)

                    if not output:
                        continue

                    if type(output) == list:
                        bootimages.extend(output)

                    elif type(output) == str:
                        bootimages.append(output)

                else:
                    bootimages.append(i)

            for bootimage in bootimages:
                OS.ui.details("Extracting boot image: %s" % bootimage)
                try:
                    z.extract(bootimage, path=tempdir)
                except:
                    OS.ui.failed('Failed to extract file: %s' % bootimage)
                    return None

        z.close()

        OS.ui.clear()

        if self.file_info.patchinfo.has_boot_image:
            for bootimage in bootimages:
                bootimageold = os.path.join(tempdir, bootimage)
                bootimagenew = self.patch_boot_image(path=bootimageold)

                if not bootimagenew:
                    return None

                os.remove(bootimageold)
                shutil.move(bootimagenew, bootimageold)
        else:
            OS.ui.info('No boot images to patch')

        OS.ui.set_task(self.tasks['PATCHING_FILES'])

        shutil.copy(os.path.join(OS.patchdir, 'dualboot.sh'), tempdir)
        autopatcher.insert_partition_info(tempdir, 'dualboot.sh',
                                          self.file_info.partconfig)

        if ap_instances:
            OS.ui.details('Running autopatchers ...')

            for ap in ap_instances:
                ret = ap.patch(tempdir, self.file_info, bootimages=bootimages)

                # Auto patchers do not have to return anything
                if ret is False:
                    OS.ui.failed(ap.error_msg)
                    return None

        OS.ui.set_task(self.tasks['COMPRESSING_ZIP_FILE'])
        OS.ui.details('Opening input and output zip files ...')

        # We can't avoid recompression, unfortunately
        # Only show progress for this stage on Android, since it's, by far, the
        # most time consuming part of the process
        new_zip_file = tempfile.mkstemp()
        z_input = zipfile.ZipFile(self.file_info.filename, 'r')
        z_output = zipfile.ZipFile(new_zip_file[1], 'w', zipfile.ZIP_DEFLATED)

        progress_current = 0
        progress_total = len(z_input.infolist()) - len(files_to_patch)
        if self.file_info.patchinfo.has_boot_image:
            progress_total -= len(bootimages)
        for root, dirs, files in os.walk(tempdir):
            progress_total += len(files)

        OS.ui.max_progress(progress_total)

        for i in z_input.infolist():
            # Skip patched files
            if i.filename in files_to_patch:
                continue
            # Boot image too
            elif self.file_info.patchinfo.has_boot_image \
                    and i.filename in bootimages:
                continue

            OS.ui.details('Adding file to zip: %s' % i.filename)
            OS.ui.progress()

            in_info = z_input.getinfo(i.filename)
            z_output.writestr(in_info, z_input.read(in_info))

        OS.ui.clear()

        z_input.close()

        for root, dirs, files in os.walk(tempdir):
            for f in files:
                OS.ui.details('Adding file to zip: %s' % f)
                OS.ui.progress()
                arcdir = os.path.relpath(root, start=tempdir)
                z_output.write(os.path.join(root, f),
                               arcname=os.path.join(arcdir, f))

        OS.ui.clear()

        z_output.close()

        os.close(new_zip_file[0])
        return new_zip_file[1]

    def cleanup(self):
        for d in self.tempdirs:
            shutil.rmtree(d)

        del self.tempdirs[:]


class PrimaryUpgradePatcher(Patcher):
    def __init__(self, file_info):
        super(PrimaryUpgradePatcher, self).__init__(file_info)

        # List of temporary directories created while patching
        self.tempdirs = list()

        self.add_task('EXTRACTING_ZIP', 'Extracting zip file')
        self.add_task('PATCHING_FILES', 'Patching files')
        self.add_task('COMPRESSING_ZIP_FILE', 'Compressing zip file')

    def start_patching(self):
        newfile = None

        try:
            if self.file_info.filetype == fileinfo.ZIP_FILE:
                newfile = self.patch_zip()

                if newfile is not None:
                    OS.ui.succeeded('Successfully patched zip file'
                                    ' for updating the primary ROM')

            elif self.file_info.filetype == fileinfo.BOOT_IMAGE:
                OS.ui.failed('Boot images do not have to be patched'
                             ' for upgrading the primary ROM')

            else:
                OS.ui.failed('Unsupported file extension')

        except Exception as e:
            OS.ui.failed(str(e))

        self.cleanup()

        return newfile

    def patch_zip(self):
        if not OS.is_android():
            OS.ui.info('--- Please wait. This may take a while ---')

        tempdir = tempfile.mkdtemp()
        self.tempdirs.append(tempdir)

        OS.ui.set_task(self.tasks['EXTRACTING_ZIP'])

        OS.ui.details('Loading zip file ...')
        z = zipfile.ZipFile(self.file_info.filename, 'r')

        try:
            z.extract(UPDATER_SCRIPT, path=tempdir)
        except:
            OS.ui.failed('Failed to extract updater-script')
            return None

        z.close()

        OS.ui.clear()

        OS.ui.set_task(self.tasks['PATCHING_FILES'])

        lines = fileio.all_lines(os.path.join(tempdir, UPDATER_SCRIPT))

        i = 0
        i += autopatcher.insert_line(
            i, EXTRACT % (PERM_TOOL, '/tmp/' + PERM_TOOL), lines)
        i += autopatcher.insert_line(
            i, EXTRACT % ('setfacl', '/tmp/setfacl'), lines)
        i += autopatcher.insert_line(
            i, EXTRACT % ('setfattr', '/tmp/setfattr'), lines)
        i += autopatcher.insert_line(
            i, EXTRACT % ('getfacl', '/tmp/getfacl'), lines)
        i += autopatcher.insert_line(
            i, EXTRACT % ('getfattr', '/tmp/getfattr'), lines)
        i += autopatcher.insert_line(
            i, MAKE_EXECUTABLE % ('/tmp/' + PERM_TOOL), lines)
        i += autopatcher.insert_line(
            i, MAKE_EXECUTABLE % '/tmp/setfacl', lines)
        i += autopatcher.insert_line(
            i, MAKE_EXECUTABLE % '/tmp/setfattr', lines)
        i += autopatcher.insert_line(
            i, MAKE_EXECUTABLE % '/tmp/getfacl', lines)
        i += autopatcher.insert_line(
            i, MAKE_EXECUTABLE % '/tmp/getfattr', lines)

        def insert_format_system(index, lines, mount):
            i = 0

            if mount:
                i += autopatcher.insert_line(index + i,
                                             MOUNT % '/system', lines)

            i += autopatcher.insert_line(index + i,
                                         PERMS_BACKUP % '/system', lines)
            i += autopatcher.insert_line(index + i, FORMAT_SYSTEM, lines)
            i += autopatcher.insert_line(index + i,
                                         PERMS_RESTORE % '/system', lines)

            if mount:
                i += autopatcher.insert_line(index + i,
                                             UNMOUNT % '/system', lines)

            return i

        def insert_format_cache(index, lines, mount):
            i = 0

            if mount:
                i += autopatcher.insert_line(index + i,
                                             MOUNT % '/cache', lines)

            i += autopatcher.insert_line(index + i,
                                         PERMS_BACKUP % '/cache', lines)
            i += autopatcher.insert_line(index + i, FORMAT_CACHE, lines)
            i += autopatcher.insert_line(index + i,
                                         PERMS_RESTORE % '/cache', lines)

            if mount:
                i += autopatcher.insert_line(index + i,
                                             UNMOUNT % '/cache', lines)

            return i

        psystem = config.get_partition(self.file_info.device, 'system')
        pcache = config.get_partition(self.file_info.device, 'cache')
        replaced_format_system = False
        replaced_format_cache = False

        i = 0
        while i < len(lines):
            if re.search(r"^\s*format\s*\(.*$", lines[i]):
                if 'system' in lines[i] or (psystem and psystem in lines[i]):
                    replaced_format_system = True
                    del lines[i]
                    i += insert_format_system(i, lines, True)

                elif 'cache' in lines[i] or (pcache and pcache in lines[i]):
                    replaced_format_cache = True
                    del lines[i]
                    i += insert_format_cache(i, lines, True)

                else:
                    i += 1

            elif re.search(r'delete_recursive\s*\([^\)]*"/system"', lines[i]):
                replaced_format_system = True
                del lines[i]
                i += insert_format_system(i, lines, False)

            elif re.search(r'delete_recursive\s*\([^\)]*"/cache"', lines[i]):
                replaced_format_cache = True
                del lines[i]
                i += insert_format_cache(i, lines, False)

            else:
                i += 1

        if not replaced_format_system:
            OS.ui.failed('The patcher could not find any /system formatting'
                         ' lines in the updater-script file.\n\nIf the file is'
                         ' a ROM, then something failed. If the file is not a'
                         ' ROM (eg. kernel or mod), it doesn\'t need to be'
                         ' patched.')
            return None

        fileio.write_lines(os.path.join(tempdir, UPDATER_SCRIPT), lines)

        OS.ui.set_task(self.tasks['COMPRESSING_ZIP_FILE'])
        OS.ui.details('Opening input and output zip files ...')

        # We can't avoid recompression, unfortunately
        # Only show progress for this stage on Android, since it's, by far, the
        # most time consuming part of the process
        new_zip_file = tempfile.mkstemp()
        z_input = zipfile.ZipFile(self.file_info.filename, 'r')
        z_output = zipfile.ZipFile(new_zip_file[1], 'w', zipfile.ZIP_DEFLATED)

        progress_current = 0
        # Five extra files
        progress_total = len(z_input.infolist()) + 5

        OS.ui.max_progress(progress_total)

        for i in z_input.infolist():
            if i.filename == UPDATER_SCRIPT:
                continue

            OS.ui.details('Adding file to zip: %s' % i.filename)
            OS.ui.progress()

            in_info = z_input.getinfo(i.filename)
            z_output.writestr(in_info, z_input.read(in_info))

        OS.ui.clear()

        z_input.close()

        OS.ui.details('Adding file to zip: ' + UPDATER_SCRIPT)
        OS.ui.progress()
        z_output.write(os.path.join(tempdir, UPDATER_SCRIPT),
                       arcname=UPDATER_SCRIPT)

        for f in [PERM_TOOL, 'setfacl', 'setfattr', 'getfacl', 'getfattr']:
            OS.ui.details('Adding file to zip: ' + f)
            OS.ui.progress()
            z_output.write(os.path.join(OS.patchdir, f), arcname=f)

        OS.ui.clear()

        z_output.close()

        os.close(new_zip_file[0])
        return new_zip_file[1]

    def cleanup(self):
        for d in self.tempdirs:
            shutil.rmtree(d)

        del self.tempdirs[:]
