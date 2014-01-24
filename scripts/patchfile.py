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

    file_info = fileinfo.FileInfo()
    file_info.set_filename(filename)
    file_info.set_device(config.get_device())

    if not file_info.is_filetype_supported():
        ui.failed('Unsupported file type')
        sys.exit(1)

    if not file_info.find_and_merge_patchinfo():
        ui.failed('Unsupported file')
        sys.exit(1)

    if not file_info.is_partconfig_supported(partition_config):
        ui.info("The %s partition configuration is not supported for this file"
                % partition_config.id)
        sys.exit(1)

    file_info.set_partconfig(partition_config)

    newfile = patcher.patch_file(file_info)

    if not newfile:
        sys.exit(1)

    file_info.move_to_target(newfile)

    if not OS.is_android():
        ui.info("Path: " + file_info.get_new_filename())

    sys.exit(0)
