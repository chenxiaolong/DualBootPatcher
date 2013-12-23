#!/usr/bin/env python3

import os
import re
import shutil
import sys
import tarfile

sys.dont_write_bytecode = True

import multiboot.exit as exit
import multiboot.fileinfo as fileinfo
import multiboot.operatingsystem as OS
import multiboot.partitionconfigs as partitionconfigs
import multiboot.patcher as patcher
import patchfile

try:
    OS.detect()
except Exception as e:
    print(str(e))
    sys.exit(1)

ramdisks = []
suffix = r"\.def$"
list_ramdisks = False
loki = False

if len(sys.argv) < 2:
    print("Usage: %s [zip file or boot.img]" % sys.argv[0])
    sys.exit(1)

if sys.argv[1] == "--list":
    list_ramdisks = True
elif sys.argv[1] == "--loki":
    loki = True
    filename = sys.argv[2]
else:
    filename = sys.argv[1]

if not list_ramdisks:
    filetype = patchfile.detect_file_type(filename)
    if filetype == "UNKNOWN":
        print("Unsupported file")
        sys.exit(1)

for root, dirs, files in os.walk(OS.ramdiskdir):
    relative_path = os.path.relpath(root, OS.ramdiskdir)
    for f in files:
        if re.search(suffix, f):
            ramdisks.append(os.path.join(relative_path, f))

if not ramdisks:
    print("No ramdisks found")
    sys.exit(1)

ramdisks.sort()

if list_ramdisks:
    for i in ramdisks:
        print(i)
    sys.exit(0)

print("Patch ramdisk in: " + filename)
print("")
print("Which type of ramdisk do you have?")
print("")

counter = 0
for i in ramdisks:
    print("%i: %s" % (counter + 1, re.sub(suffix, "", i)))
    counter += 1

print("")
choice = input("Choice: ")

try:
    choice = int(choice)
    choice -= 1
    if choice < 0 or choice >= counter:
        print("Invalid choice")
        sys.exit(1)

    partition_configs = partitionconfigs.get()
    partition_config = None
    for i in partition_configs:
        if i.id == 'dualboot':
            partition_config = i

    file_info = fileinfo.FileInfo()
    file_info.ramdisk = ramdisks[choice]
    file_info.bootimg = "boot.img"
    if loki:
        file_info.loki = True
        file_info.bootimg = "boot.lok"

    print("")
    print("Replacing ramdisk with " + file_info.ramdisk)

    if filetype == "img":
        new_file = patcher.patch_boot_image(filename, file_info,
                                            partition_config)
        new_path = re.sub(r"\.img$", "_dualboot.img", filename)
    elif filetype == "zip":
        new_file = patcher.patch_zip(filename, file_info, partition_config)
        new_path = re.sub(r"\.zip$", "_dualboot.zip", filename)

    shutil.move(new_file, new_path)
    print("Path: " + new_path)
    exit.exit(0)

except ValueError:
    print("Invalid choice")
    sys.exit(1)
