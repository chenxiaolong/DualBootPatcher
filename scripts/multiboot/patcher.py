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

import multiboot.ui.dummyui as dummyui
import multiboot.autopatcher as autopatcher
import multiboot.bootimage as bootimage
import multiboot.config as config
import multiboot.fileinfo as fileinfo
import multiboot.fileio as fileio
import multiboot.minicpio.minicpio as minicpio
import multiboot.operatingsystem as OS
import multiboot.patch as patch
import multiboot.ramdisk as ramdisk

import gzip
import os
import shutil
import tempfile
import zipfile

loki_msg = ("The boot image was unloki'd in order to be patched. "
            "*** Remember to flash loki-doki "
            "if you have a locked bootloader ***")

new_ui = None

tasks = dict()
tasks['EXTRACTING_ZIP']       = 'Extracting zip file'
tasks['PATCHING_RAMDISK']     = 'Patching ramdisk'
tasks['PATCHING_FILES']       = 'Patching files'
tasks['COMPRESSING_ZIP_FILE'] = 'Compressing zip file'


def add_tasks(file_info):
    if new_ui:
        ui = new_ui
    else:
        ui = OS.ui

    if file_info.filetype == fileinfo.ZIP_FILE:
        ui.add_task(tasks['EXTRACTING_ZIP'])

    if file_info.filetype == fileinfo.BOOT_IMAGE \
            or file_info.patchinfo.has_boot_image:
        ui.add_task(tasks['PATCHING_RAMDISK'])

    if file_info.filetype == fileinfo.ZIP_FILE:
        ui.add_task(tasks['PATCHING_FILES'])
        ui.add_task(tasks['COMPRESSING_ZIP_FILE'])


def set_task(task):
    if new_ui:
        ui = new_ui
    else:
        ui = OS.ui
    ui.set_task(tasks[task])


def patch_boot_image(file_info, path=None):
    # If file_info represents a boot image
    if not path:
        path = file_info.filename

    tempdirs = list()

    if new_ui:
        ui = new_ui
    else:
        ui = OS.ui

    set_task('PATCHING_RAMDISK')

    tempdir = tempfile.mkdtemp()
    tempdirs.append(tempdir)

    ui.details('Unpacking boot image ...')

    bootimageinfo = bootimage.extract(path, tempdir, device=file_info.device)
    if not bootimageinfo:
        ui.failed(bootimage.error_msg)
        cleanup(tempdirs)
        return None

    selinux = config.get_selinux(file_info.device)
    if selinux is not None:
        bootimageinfo.cmdline += ' androidboot.selinux=' + selinux

    target_ramdisk = os.path.join(tempdir, 'ramdisk.cpio')
    target_kernel = os.path.join(tempdir, 'kernel.img')

    shutil.move(bootimageinfo.kernel, target_kernel)

    # Minicpio
    ui.details('Loading ramdisk with minicpio ...')
    cf = minicpio.CpioFile()
    with gzip.open(bootimageinfo.ramdisk, 'rb') as f:
        cf.load(f.read())

    os.remove(bootimageinfo.ramdisk)

    # Patch ramdisk
    if not file_info.patchinfo.ramdisk:
        ui.info('No ramdisk patch specified')
        return None

    ui.details('Modifying ramdisk ...')

    try:
        ramdisk.process_def(
            os.path.join(OS.ramdiskdir, file_info.patchinfo.ramdisk),
            cf,
            file_info.partconfig
        )
    except Exception as e:
        ui.failed('Failed to patch ramdisk: ' + str(e))
        cleanup(tempdirs)
        return None

    # Copy init.multiboot.mounting.sh
    shutil.copy(
        os.path.join(OS.ramdiskdir, 'init.multiboot.mounting.sh'),
        tempdir
    )
    autopatcher.insert_partition_info(
        tempdir, 'init.multiboot.mounting.sh',
        file_info.partconfig, target_path_only=True
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

    # Copy patched init if needed
    if file_info.patchinfo.patched_init is not None:
        cf.add_file(
            os.path.join(OS.ramdiskdir, 'init',
                         file_info.patchinfo.patched_init),
            name='init',
            perms=0o755
        )

    # Create gzip compressed ramdisk
    ui.details('Creating gzip compressed ramdisk with minicpio ...')

    target_ramdisk = target_ramdisk + ".gz"

    with gzip.open(target_ramdisk, 'wb') as f_out:
        data = cf.create()

        if data is not None:
            f_out.write(data)
        else:
            ui.failed('Failed to create gzip compressed ramdisk')
            cleanup(tempdirs)
            return None

    ui.details('Running mkbootimg ...')

    bootimageinfo.kernel = target_kernel
    bootimageinfo.ramdisk = target_ramdisk

    new_boot_image = tempfile.mkstemp()

    if not bootimage.create(bootimageinfo, new_boot_image[1]):
        ui.failed('Failed to create boot image: ' + bootimage.error_msg)
        cleanup(tempdirs)
        os.close(new_boot_image[0])
        os.remove(new_boot_image[1])
        return None

    os.remove(target_kernel)
    os.remove(target_ramdisk)

    cleanup(tempdirs)
    os.close(new_boot_image[0])
    return new_boot_image[1]


def patch_zip(file_info):
    tempdirs = list()

    if new_ui:
        ui = new_ui
    else:
        ui = OS.ui

    if not OS.is_android():
        ui.info('--- Please wait. This may take a while ---')

    files_to_patch = []
    if type(file_info.patchinfo.patch) == str and \
            file_info.patchinfo.patch != "":
        files_to_patch = patch.files_in_patch(file_info.patchinfo.patch)
    elif type(file_info.patchinfo.patch) == list:
        for i in file_info.patchinfo.patch:
            if type(i) == str and i != "":
                files_to_patch.extend(patch.files_in_patch(i))

    if file_info.patchinfo.extract:
        for i in file_info.patchinfo.extract:
            if callable(i):
                output = i()

                if type(output) == list:
                    files_to_patch.extend(output)

                elif type(output) == str:
                    files_to_patch.append(output)

            elif type(i) == str:
                files_to_patch.append(i)

    tempdir = tempfile.mkdtemp()
    tempdirs.append(tempdir)

    set_task('EXTRACTING_ZIP')

    ui.details('Loading zip file ...')
    z = zipfile.ZipFile(file_info.filename, 'r')

    for f in files_to_patch:
        ui.details("Extracting file to be patched: %s" % f)
        try:
            z.extract(f, path=tempdir)
        except:
            ui.failed('Failed to extract file: %s' % f)
            cleanup(tempdirs)
            return None

    if file_info.patchinfo.has_boot_image:
        bootimages = list()

        for i in file_info.patchinfo.bootimg:
            if callable(i):
                output = i(file_info.filename)

                if not output:
                    continue

                if type(output) == list:
                    bootimages.extend(output)

                elif type(output) == str:
                    bootimages.append(output)

            else:
                bootimages.append(i)

        for bootimage in bootimages:
            ui.details("Extracting boot image: %s" % bootimage)
            try:
                z.extract(bootimage, path=tempdir)
            except:
                ui.failed('Failed to extract file: %s' % bootimage)
                cleanup(tempdirs)
                return None

    z.close()

    ui.clear()

    if file_info.patchinfo.has_boot_image:
        for bootimage in bootimages:
            bootimageold = os.path.join(tempdir, bootimage)
            bootimagenew = patch_boot_image(file_info, path=bootimageold)

            if not bootimagenew:
                cleanup(tempdirs)
                return None

            os.remove(bootimageold)
            shutil.move(bootimagenew, bootimageold)
    else:
        ui.info('No boot images to patch')

    set_task('PATCHING_FILES')

    shutil.copy(os.path.join(OS.patchdir, 'dualboot.sh'), tempdir)
    autopatcher.insert_partition_info(tempdir, 'dualboot.sh',
                                      file_info.partconfig)

    if file_info.patchinfo.patch:
        ui.details('Running patch functions ...')

        for i in file_info.patchinfo.patch:
            if callable(i):
                if file_info.patchinfo.has_boot_image:
                    i(tempdir,
                      bootimg=bootimages,
                      device_check=file_info.patchinfo.device_check,
                      partition_config=file_info.partconfig,
                      device=file_info.device)
                else:
                    i(tempdir,
                      device_check=file_info.patchinfo.device_check,
                      partition_config=file_info.partconfig,
                      device=file_info.device)

            elif type(i) == str:
                if not patch.apply_patch(i, tempdir):
                    ui.failed(patch.error_msg)
                    cleanup(tempdirs)
                    return None

    set_task('COMPRESSING_ZIP_FILE')
    ui.details('Opening input and output zip files ...')

    # We can't avoid recompression, unfortunately
    # Only show progress for this stage on Android, since it's, by far, the
    # most time consuming part of the process
    new_zip_file = tempfile.mkstemp()
    z_input = zipfile.ZipFile(file_info.filename, 'r')
    z_output = zipfile.ZipFile(new_zip_file[1], 'w', zipfile.ZIP_DEFLATED)

    progress_current = 0
    progress_total = len(z_input.infolist()) - len(files_to_patch)
    if file_info.patchinfo.has_boot_image:
        progress_total -= len(bootimages)
    for root, dirs, files in os.walk(tempdir):
        progress_total += len(files)

    ui.max_progress(progress_total)

    for i in z_input.infolist():
        # Skip patched files
        if i.filename in files_to_patch:
            continue
        # Boot image too
        elif file_info.patchinfo.has_boot_image and i.filename in bootimages:
            continue

        ui.details('Adding file to zip: %s' % i.filename)
        ui.progress()

        in_info = z_input.getinfo(i.filename)
        z_output.writestr(in_info, z_input.read(in_info))

    ui.clear()

    z_input.close()

    for root, dirs, files in os.walk(tempdir):
        for f in files:
            ui.details('Adding file to zip: %s' % f)
            ui.progress()
            arcdir = os.path.relpath(root, start=tempdir)
            z_output.write(os.path.join(root, f),
                           arcname=os.path.join(arcdir, f))

    ui.clear()

    z_output.close()

    cleanup(tempdirs)
    os.close(new_zip_file[0])
    return new_zip_file[1]


def patch_file(file_info):
    # Some sanity checks
    if not file_info.filename or not os.path.exists(file_info.filename):
        raise Exception('%s does not exist' % file_info.filename)

    if not file_info.filetype:
        raise Exception('File type not set')

    if new_ui:
        ui = new_ui
    else:
        ui = OS.ui

    add_tasks(file_info)

    if file_info.filetype == fileinfo.ZIP_FILE:
        newfile = patch_zip(file_info)
        if newfile is not None:
            # if file_info.loki:
            #     ui.succeeded('Successfully patched zip. ' + loki_msg)
            # else:
            ui.succeeded('Successfully patched zip')
    elif file_info.filetype == fileinfo.BOOT_IMAGE:
        newfile = patch_boot_image(file_info)
        if newfile is not None:
            # if file_info.loki:
            #     ui.succeeded('Successfully patched Loki\'d boot image. ' +
            #                  loki_msg)
            # else:
            ui.succeeded('Successfully patched boot image')
    else:
        raise Exception('Unsupported file extension')

    return newfile


def set_ui(ui):
    global new_ui
    if not ui:
        new_ui = dummyui
    else:
        new_ui = ui


def cleanup(dirs):
    for d in dirs:
        shutil.rmtree(d)
