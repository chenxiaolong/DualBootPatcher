#!/usr/bin/env python3

# For Qualcomm based Samsung Galaxy S4 only!

# Python 2 compatibility
from __future__ import print_function

try:
    import multiboot.operatingsystem as OS
except Exception as e:
    print(str(e))
    sys.exit(1)

import multiboot.config as config
import multiboot.fileinfo as fileinfo
import multiboot.partitionconfigs as partitionconfigs
import multiboot.patcher as patcher

import os
import re
import shutil
import sys


ui = OS.ui


def detect_file_type(path):
    if path.endswith(".zip"):
        return "zip"
    elif path.endswith(".img"):
        return "img"
    else:
        return "UNKNOWN"


if __name__ == "__main__":
    if len(sys.argv) < 2:
        ui.info("Usage: %s [zip file or boot.img] [multiboot type]"
                % sys.argv[0])
        sys.exit(1)

    filename = sys.argv[1]
    if len(sys.argv) == 2:
        config_name = 'dualboot'
    else:
        config_name = sys.argv[2]

    partition_configs = partitionconfigs.get()
    partition_config = None
    for i in partition_configs:
        if i.id == config_name:
            partition_config = i

    if not partition_config:
        ui.info("Multiboot config %s does not exist!" % config_name)
        sys.exit(1)

    if not os.path.exists(filename):
        ui.info("%s does not exist!" % filename)
        sys.exit(1)

    filename = os.path.abspath(filename)
    filetype = detect_file_type(filename)
    file_info = fileinfo.get_info(filename, config.get_device())

    if file_info and \
            (('all' not in file_info.configs
              and partition_config.id not in file_info.configs)
             or '!' + partition_config.id in file_info.configs):
        file_info = None
        ui.info("The %s partition configuration is not supported for this file"
                % partition_config.id)

    loki_msg = ("The boot image was unloki'd in order to be patched. "
                "*** Remember to flash loki-doki "
                "if you have a locked bootloader ***")

    if filetype == "UNKNOWN":
        ui.failed("Unsupported file")
        sys.exit(1)

    if filetype == "zip":
        if not file_info:
            ui.failed("Unsupported zip")
            sys.exit(1)

        # Patch zip and get path to patched zip
        patcher.add_tasks(file_info)
        newfile = patcher.patch_zip(filename, file_info, partition_config)

        if file_info.loki:
            ui.succeeded("Successfully patched zip. " + loki_msg)
        else:
            ui.succeeded("Successfully patched zip")

        newpath = re.sub(r"\.zip$", "_%s.zip" % partition_config.id, filename)
        shutil.copyfile(newfile, newpath)
        os.remove(newfile)

        if not OS.is_android():
            ui.info("Path: " + newpath)

    elif filetype == "img":
        patcher.add_tasks(file_info)
        newfile = patcher.patch_boot_image(filename, file_info,
                                           partition_config)

        if file_info.loki:
            ui.succeeded("Successfully patched boot image. " + loki_msg)
        else:
            ui.succeeded("Successfully patched boot image")

        newpath = re.sub(r"\.img$", "_%s.img" % partition_config.id, filename)
        shutil.copyfile(newfile, newpath)
        os.remove(newfile)

        if not OS.is_android():
            ui.info("Path: " + newpath)

    sys.exit(0)
