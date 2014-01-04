import multiboot.ui.dummyui as dummyui
import multiboot.autopatcher as autopatcher
import multiboot.bootimage as bootimage
import multiboot.config as config
import multiboot.cpio as cpio
import multiboot.exit as exit
import multiboot.fileio as fileio
import multiboot.operatingsystem as OS
import multiboot.patch as patch
import multiboot.ramdisk as ramdisk

import gzip
import os
import shutil
import tempfile
import zipfile

new_ui = None

tasks = dict()
tasks['EXTRACTING_ZIP']       = 'Extracting zip file'
tasks['PATCHING_RAMDISK']     = 'Patching ramdisk'
tasks['COMPRESSING_RAMDISK']  = 'Compressing ramdisk'
tasks['CREATING_BOOT_IMAGE']  = 'Creating boot image'
tasks['PATCHING_FILES']       = 'Patching files'
tasks['COMPRESSING_ZIP_FILE'] = 'Compressing zip file'


def add_tasks(file_info):
    if new_ui:
        ui = new_ui
    else:
        ui = OS.ui

    ui.add_task(tasks['EXTRACTING_ZIP'])

    if file_info.has_boot_image:
        ui.add_task(tasks['PATCHING_RAMDISK'])
        ui.add_task(tasks['COMPRESSING_RAMDISK'])
        ui.add_task(tasks['CREATING_BOOT_IMAGE'])

    ui.add_task(tasks['PATCHING_FILES'])
    ui.add_task(tasks['COMPRESSING_ZIP_FILE'])


def set_task(task):
    if new_ui:
        ui = new_ui
    else:
        ui = OS.ui
    ui.set_task(tasks[task])


def patch_boot_image(boot_image, file_info, partition_config):
    if new_ui:
        ui = new_ui
    else:
        ui = OS.ui

    set_task('PATCHING_RAMDISK')

    tempdir = tempfile.mkdtemp()
    exit.remove_on_exit(tempdir)

    if file_info.loki:
        ui.details('Unpacking loki boot image ...')
    else:
        ui.details('Unpacking boot image ...')

    bootimageinfo = bootimage.extract(boot_image, tempdir, file_info.loki)
    if not bootimageinfo:
        ui.failed(bootimage.error_msg)
        exit.exit(1)

    selinux = config.get_selinux()
    if selinux is not None:
        bootimageinfo.cmdline += ' androidboot.selinux=' + selinux

    target_ramdisk = os.path.join(tempdir, 'ramdisk.cpio')
    target_kernel = os.path.join(tempdir, 'kernel.img')

    shutil.move(bootimageinfo.kernel, target_kernel)

    extracted = os.path.join(tempdir, 'extracted')
    os.mkdir(extracted)

    # Decompress ramdisk
    ui.details('Extracting ramdisk using cpio ...')
    with gzip.open(bootimageinfo.ramdisk, 'rb') as f_in:
        if not cpio.extract(extracted, data=f_in.read()):
            ui.command_error(output=cpio.output, error=cpio.error)
            ui.failed('Failed to extract ramdisk using cpio')
            exit.exit(1)

    os.remove(bootimageinfo.ramdisk)

    # Patch ramdisk
    if not file_info.ramdisk:
        ui.info('No ramdisk patch specified')
        return None

    ui.details('Modifying ramdisk ...')

    ramdisk.process_def(
        os.path.join(OS.ramdiskdir, file_info.ramdisk),
        extracted,
        partition_config
    )

    # Copy init.multiboot.mounting.sh
    shutil.copy(
        os.path.join(OS.ramdiskdir, 'init.multiboot.mounting.sh'),
        extracted
    )
    autopatcher.insert_partition_info(
        extracted, 'init.multiboot.mounting.sh',
        partition_config, target_path_only=True
    )

    # Copy busybox
    shutil.copy(
        os.path.join(OS.ramdiskdir, 'busybox-static'),
        os.path.join(extracted, 'sbin')
    )

    # Copy new init if needed
    if file_info.need_new_init:
        shutil.copy(
            os.path.join(OS.ramdiskdir, 'init'),
            extracted
        )

    # Create gzip compressed ramdisk
    set_task('COMPRESSING_RAMDISK')
    ramdisk_files = []
    for root, dirs, files in os.walk(extracted):
        for d in dirs:
            if os.path.samefile(root, extracted):
                # cpio, for whatever reason, creates a directory called '.'

                ramdisk_files.append(fileio.unix_path(d))

                ui.details('Adding directory to ramdisk: %s' % d)
            else:
                relative_dir = os.path.relpath(root, extracted)

                ramdisk_files.append(fileio.unix_path(
                    os.path.join(relative_dir, d)))

                ui.details('Adding directory to ramdisk: %s'
                           % os.path.join(relative_dir, d))

        for f in files:
            if os.path.samefile(root, extracted):
                # cpio, for whatever reason, creates a directory called '.'

                ramdisk_files.append(fileio.unix_path(f))

                ui.details('Adding file to ramdisk: %s' % f)
            else:
                relative_dir = os.path.relpath(root, extracted)

                ramdisk_files.append(fileio.unix_path(
                    os.path.join(relative_dir, f)))

                ui.details('Adding file to ramdisk: %s'
                           % os.path.join(relative_dir, f))

    ui.details('Creating gzip compressed ramdisk with cpio ...')

    target_ramdisk = target_ramdisk + ".gz"

    with gzip.open(target_ramdisk, 'wb') as f_out:
        data = cpio.create(extracted, ramdisk_files)

        if data is not None:
            f_out.write(data)
        else:
            ui.command_error(output=cpio.output, error=cpio.error)
            ui.failed('Failed to create gzip compressed ramdisk')
            exit.exit(1)

    set_task('CREATING_BOOT_IMAGE')
    ui.details('Running mkbootimg ...')

    bootimageinfo.kernel = target_kernel
    bootimageinfo.ramdisk = target_ramdisk

    if not bootimage.create(bootimageinfo,
                            os.path.join(tempdir, 'complete.img')):
        ui.failed('Failed to create boot image: ' + bootimage.error_msg)
        exit.exit(1)

    os.remove(target_kernel)
    os.remove(target_ramdisk)

    return os.path.join(tempdir, 'complete.img')


def patch_zip(zip_file, file_info, partition_config):
    if new_ui:
        ui = new_ui
    else:
        ui = OS.ui

    if not OS.is_android():
        ui.info('--- Please wait. This may take a while ---')

    files_to_patch = []
    if type(file_info.patch) == str and file_info.patch != "":
        files_to_patch = patch.files_in_patch(file_info.patch)
    elif type(file_info.patch) == list:
        for i in file_info.patch:
            if type(i) == str and i != "":
                files_to_patch.extend(patch.files_in_patch(i))

    if file_info.extract:
        temp = file_info.extract
        if type(temp) == str or callable(temp):
            temp = [temp]

        for i in temp:
            if callable(i):
                output = i()

                if type(output) == list:
                    files_to_patch.extend(output)

                elif type(output) == str:
                    files_to_patch.append(output)

            elif type(i) == list:
                files_to_patch.extend(i)

            elif type(i) == str:
                files_to_patch.append(i)

    tempdir = tempfile.mkdtemp()
    exit.remove_on_exit(tempdir)

    set_task('EXTRACTING_ZIP')

    ui.details('Loading zip file ...')
    z = zipfile.ZipFile(zip_file, 'r')

    for f in files_to_patch:
        ui.details("Extracting file to be patched: %s" % f)
        try:
            z.extract(f, path=tempdir)
        except:
            ui.failed('Failed to extract file: %s' % f)
            exit.exit(1)

    if file_info.has_boot_image:
        ui.details("Extracting boot image: %s" % file_info.bootimg)
        try:
            z.extract(file_info.bootimg, path=tempdir)
        except:
            ui.failed('Failed to extract file: %s' % file_info.bootimg)
            exit.exit(1)

    z.close()

    ui.clear()

    if file_info.has_boot_image:
        boot_image = os.path.join(tempdir, file_info.bootimg)
        new_boot_image = patch_boot_image(boot_image, file_info,
                                          partition_config)

        os.remove(boot_image)
        shutil.move(new_boot_image, boot_image)
    else:
        ui.info('No boot image to patch')

    set_task('PATCHING_FILES')

    shutil.copy(os.path.join(OS.patchdir, 'dualboot.sh'), tempdir)
    autopatcher.insert_partition_info(tempdir, 'dualboot.sh', partition_config)

    if file_info.patch:
        ui.details('Running patch functions ...')

        temp = file_info.patch
        if type(temp) == str or callable(temp):
            temp = [temp]

        for i in temp:
            if callable(i):
                i(tempdir,
                  bootimg=file_info.bootimg,
                  device_check=file_info.device_check,
                  partition_config=partition_config)

            elif type(i) == list:
                for j in i:
                    if not patch.apply_patch(j, tempdir):
                        ui.failed(patch.error_msg)
                        exit.exit(1)

            elif type(i) == str:
                if not patch.apply_patch(i, tempdir):
                    ui.failed(patch.error_msg)
                    exit.exit(1)

    set_task('COMPRESSING_ZIP_FILE')
    ui.details('Opening input and output zip files ...')

    # We can't avoid recompression, unfortunately
    # Only show progress for this stage on Android, since it's, by far, the
    # most time consuming part of the process
    new_zip_file = os.path.join(tempdir, 'complete.zip')
    z_input = zipfile.ZipFile(zip_file, 'r')
    z_output = zipfile.ZipFile(new_zip_file, 'w', zipfile.ZIP_DEFLATED)

    progress_current = 0
    progress_total = len(z_input.infolist()) - len(files_to_patch) - 1
    if file_info.has_boot_image:
        progress_total -= 1
    for root, dirs, files in os.walk(tempdir):
        progress_total += len(files)

    ui.max_progress(progress_total)

    for i in z_input.infolist():
        # Skip patched files
        if i.filename in files_to_patch:
            continue
        # Boot image too
        elif i.filename == file_info.bootimg:
            continue

        ui.details('Adding file to zip: %s' % i.filename)
        ui.progress()
        z_output.writestr(i.filename, z_input.read(i.filename))

    ui.clear()

    z_input.close()

    for root, dirs, files in os.walk(tempdir):
        for f in files:
            if f == 'complete.zip':
                continue
            ui.details('Adding file to zip: %s' % f)
            ui.progress()
            arcdir = os.path.relpath(root, start=tempdir)
            z_output.write(os.path.join(root, f),
                           arcname=os.path.join(arcdir, f))

    ui.clear()

    z_output.close()

    return new_zip_file


def set_ui(ui):
    if not ui:
        new_ui = dummyui
    else:
        new_ui = ui
