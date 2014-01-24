#!/usr/bin/env python3

import os
import re
import shutil
import sys

sys.dont_write_bytecode = True

try:
    import multiboot.operatingsystem as OS
except Exception as e:
    print(str(e))
    sys.exit(1)

import multiboot.autopatcher as autopatcher
import multiboot.config as config
import multiboot.fileinfo as fileinfo
import multiboot.partitionconfigs as partitionconfigs
import multiboot.patcher as patcher
import multiboot.ramdisk as ramdisk
import common
import patchfile

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

file_info = fileinfo.FileInfo()
file_info.set_filename(filename)
file_info.set_device(config.get_device())

if not file_info.is_filetype_supported():
    print('Unsupported file type')
    sys.exit(1)

supported = file_info.find_and_merge_patchinfo()

if not supported:
    print("File not supported! Will attempt auto-patching ...")
    print("")

if askramdisk or not file_info:
    try:
        ramdisks, choice = common.ask_ramdisk()
    except Exception as e:
        print(str(e))
        sys.exit(1)

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

file_info.set_partconfig(partition_configs[choice])

new_file = patcher.patch_file(file_info)

if not new_file:
    sys.exit(1)

file_info.move_to_target(new_file)

print("Path: " + file_info.get_new_filename())
sys.exit(0)
