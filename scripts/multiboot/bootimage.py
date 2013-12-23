import multiboot.cmd as cmd
import multiboot.debug as debug
import multiboot.fileio as fileio
import multiboot.operatingsystem as OS
import multiboot.standalone.unlokibootimg

import os

output         = None
error          = None
error_msg      = None


class BootImageInfo:
    def __init__(self):
        self.base           = None
        self.cmdline        = None
        self.pagesize       = None
        # FIXME: For Galaxy S4 only
        self.ramdisk_offset = "0x02000000"

        # Paths
        self.kernel         = None
        self.ramdisk        = None


def create(info, output_file):
    global output, error, error_msg

    command = [OS.mkbootimg]

    if info.kernel is not None:
        command.append('--kernel')
        command.append(info.kernel)

    if info.ramdisk is not None:
        command.append('--ramdisk')
        command.append(info.ramdisk)

    if info.cmdline is not None:
        command.append('--cmdline')
        command.append(info.cmdline)

    if info.base is not None:
        command.append('--base')
        command.append(info.base)

    if info.pagesize is not None:
        command.append('--pagesize')
        command.append(info.pagesize)

    if info.ramdisk_offset is not None:
        command.append('--ramdisk_offset')
        command.append(info.ramdisk_offset)

    command.append('--output')
    command.append(output_file)

    exit_code, output, error = cmd.run_command(
        command
    )

    if exit_code != 0:
        return False
    else:
        return True


def extract(boot_image, output_dir, loki):
    global output, error, error_msg

    if loki:
        try:
            if not OS.is_android() and not debug.is_debug():
                unlokibootimg.show_output = False
            unlokibootimg.extract(boot_image, output_dir)
        except Exception as e:
            error_msg = str(e)
            return False

    else:
        exit_code, output, error = cmd.run_command(
            [OS.unpackbootimg,
             '-i', boot_image,
             '-o', output_dir]
        )

        if exit_code != 0:
            error_msg = 'Failed to extract boot image'
            return None

    info = BootImageInfo()

    prefix        = os.path.join(output_dir, os.path.split(boot_image)[1])
    info.base     = '0x' + fileio.first_line(prefix + '-base')
    info.cmdline  =        fileio.first_line(prefix + '-cmdline')
    info.pagesize =        fileio.first_line(prefix + '-pagesize')
    os.remove(prefix + '-base')
    os.remove(prefix + '-cmdline')
    os.remove(prefix + '-pagesize')

    info.kernel   = prefix + '-zImage'
    info.ramdisk  = prefix + '-ramdisk.gz'

    return info
