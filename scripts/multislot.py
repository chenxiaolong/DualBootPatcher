#!/usr/bin/env python3

import os
import re
import shutil
import sys

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

if len(sys.argv) < 2:
    print("Usage: %s [zip file or boot.img]" % sys.argv[0])
    sys.exit(1)

filename = sys.argv[1]

filetype = patchfile.detect_file_type(filename)
if filetype == "UNKNOWN":
    print("Unsupported file")
    sys.exit(1)

file_info = fileinfo.get_info(filename, 'jflte')
if not file_info:
    print("Unsupported file")
    sys.exit(1)

unsupported_configs = []
partition_configs = []
partition_configs_raw = partitionconfigs.get()
print(partition_configs_raw)

for i in partition_configs_raw:
    if (i.id in file_info.configs or 'all' in file_info.configs) \
            and '!' + i.id not in file_info.configs:
        partition_configs.append(i)
    else:
        unsupported_configs.append(i)

if not partition_configs:
    print("No usable partition configurations found")
    sys.exit(1)

print("Choose a partition configuration to use:")
print("")

counter = 0
for i in partition_configs:
    print("%i: %s" % (counter + 1, i.name))
    print("\t- %s" % i.description)
    counter += 1

for i in unsupported_configs:
    print("*: %s (UNSUPPORTED)" % i.name)
    print("\t- %s" % i.description)

print("")
choice = input("Choice: ")

try:
    choice = int(choice)
    choice -= 1
    if choice < 0 or choice >= counter:
        print("Invalid choice")
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

except ValueError:
    print("Invalid choice")
    sys.exit(1)
