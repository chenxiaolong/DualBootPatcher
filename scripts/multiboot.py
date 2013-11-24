#!/usr/bin/env python3

import os
import re
import shutil
import sys

sys.dont_write_bytecode = True

import patchfile
import partitionconfigs

if len(sys.argv) < 2:
  print("Usage: %s [zip file or boot.img]" % sys.argv[0])
  sys.exit(1)

filename = sys.argv[1]

filetype = patchfile.detect_file_type(filename)
if filetype == "UNKNOWN":
  print("Unsupported file")
  sys.exit(1)

partition_configs = partitionconfigs.get_partition_configs()

if not partition_configs:
  print("No partition configurations found")
  sys.exit(1)

print("Choose a partition configuration to use:")
print("")

counter = 0
for i in partition_configs:
  print("%i: %s" % (counter + 1, i.name))
  print("\t- %s" % i.description)
  counter += 1

print("")
choice = input("Choice: ")

try:
  choice = int(choice)
  choice -= 1
  if choice < 0 or choice >= counter:
    print("Invalid choice")
    sys.exit(1)

  partition_config = partition_configs[choice]

  file_info = patchfile.get_file_info(filename)

  if filetype == "img":
    new_file = patchfile.patch_boot_image(filename, file_info, partition_config)
    new_path = re.sub(r"\.img$", "_%s.img" % partition_config.id, filename)
  elif filetype == "zip":
    new_file = patchfile.patch_zip(filename, file_info, partition_config)
    new_path = re.sub(r"\.zip$", "_%s.zip" % partition_config.id, filename)

  shutil.copyfile(new_file, new_path)
  os.remove(new_file)
  print("Path: " + new_path)
  patchfile.clean_up_and_exit(0)

except ValueError:
  print("Invalid choice")
  sys.exit(1)
