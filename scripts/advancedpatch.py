#!/usr/bin/env python3

import os
import re
import shutil
import sys

sys.dont_write_bytecode = True

import multiboot.autopatcher as autopatcher
import multiboot.config as config
import multiboot.exit as exit
import multiboot.fileinfo as fileinfo
import multiboot.operatingsystem as OS
import multiboot.partitionconfigs as partitionconfigs
import multiboot.patcher as patcher
import multiboot.ramdisk as ramdisk
import common
import patchfile

try:
    OS.detect()
except Exception as e:
    print(str(e))
    sys.exit(1)

# Choices: list, patch
action = 'patch'
loki = False
askramdisk = False
filename = None

if len(sys.argv) < 2:
    print("Usage: %s [zip file or boot.img] [--loki] [--askramdisk] [--listramdisks]" % sys.argv[0])
    sys.exit(1)

for i in sys.argv:
    if i == '--loki':
        loki = True
    elif i == '--askramdisk':
        askramdisk = True
    elif i == '--listramdisks':
        action = 'list'
    elif i.startswith('--'):
        print("Invalid argument: " + i)
        sys.exit(1)
    else:
        filename = i

# List ramdisks if that was the action
if action == 'list':
    for i in ramdisk.list_ramdisks():
        print(i)
    sys.exit(1)

if not filename:
    print("No file specified")
    sys.exit(1)

filetype = patchfile.detect_file_type(filename)
if filetype == "UNKNOWN":
    print("Unsupported file")
    sys.exit(1)

# Ask for ramdisk if it's unsupported
file_info = fileinfo.get_info(filename, config.get_device())
if not file_info:
    print("File not supported! Will attempt auto-patching...")
    print("")

if askramdisk or not file_info:
    try:
        ramdisks, choice = common.ask_ramdisk()
    except Exception as e:
        print(str(e))
        sys.exit(1)

    file_info = fileinfo.FileInfo()
    file_info.ramdisk = ramdisks[choice]
    if loki:
        file_info.loki = True
        file_info.bootimg = 'boot.lok'
    else:
        file_info.bootimg = 'boot.img'
    file_info.patch = autopatcher.auto_patch
    file_info.extract = autopatcher.files_to_auto_patch

    print("")
    print("Replacing ramdisk with " + file_info.ramdisk)

try:
    partition_configs, choice = common.ask_partconfig(file_info)
except Exception as e:
    print(str(e))
    sys.exit(1)

partition_config = partition_configs[choice]

if filetype == "img":
    new_file = patcher.patch_boot_image(filename, file_info, partition_config)
    new_path = re.sub(r"\.img$", "_%s.img" % partition_config.id, filename)
elif filetype == "zip":
    new_file = patcher.patch_zip(filename, file_info, partition_config)
    new_path = re.sub(r"\.zip$", "_%s.zip" % partition_config.id, filename)

shutil.copyfile(new_file, new_path)
os.remove(new_file)
print("Path: " + new_path)
exit.exit(0)
