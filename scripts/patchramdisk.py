#!/usr/bin/env python3

import os
import re
import shutil
import sys
import tarfile

sys.dont_write_bytecode = True

import fileinfo
import patchfile

ramdisks = []
suffix = r"\.def$"
list_ramdisks = False

if len(sys.argv) < 2:
  print("Usage: %s [zip file or boot.img]" % sys.argv[0])
  sys.exit(1)

if sys.argv[1] == "--list":
  list_ramdisks = True
else:
  filename = sys.argv[1]

  filetype = patchfile.detect_file_type(filename)
  if filetype == "UNKNOWN":
    print("Unsupported file")
    sys.exit(1)

for root, dirs, files in os.walk(patchfile.ramdiskdir):
  relative_path = os.path.relpath(root, patchfile.ramdiskdir)
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

  file_info = fileinfo.FileInfo()
  file_info.ramdisk = ramdisks[choice]
  file_info.bootimg = "boot.img"

  print("")
  print("Replacing ramdisk with " + file_info.ramdisk)

  if filetype == "img":
    new_file = patchfile.patch_boot_image(filename, file_info)
    new_path = re.sub(r"\.img$", "_dualboot.img", filename)
  elif filetype == "zip":
    new_file = patchfile.patch_zip(filename, file_info)
    new_path = re.sub(r"\.zip$", "_dualboot.zip", filename)

  shutil.move(new_file, new_path)
  print("Path: " + new_path)
  patchfile.clean_up_and_exit(0)

except ValueError:
  print("Invalid choice")
  sys.exit(1)
