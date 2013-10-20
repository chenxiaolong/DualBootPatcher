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
suffix = r"\.dualboot\.cpio$"
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

if sys.hexversion >= 50528256: # Python 3.3
  with tarfile.open(os.path.join(patchfile.ramdiskdir, "ramdisks.tar.xz")) as f:
    ramdisks = f.getnames()
else:
  xz = "xz"
  if os.name == "nt":
    xz = os.path.join(patchfile.binariesdir, "xz.exe")

  patchfile.run_command(
    [ xz, '-d', '-k', '-f', os.path.join(patchfile.ramdiskdir, "ramdisks.tar.xz") ]
  )

  with tarfile.open(os.path.join(patchfile.ramdiskdir, "ramdisks.tar")) as f:
    ramdisks = f.getnames()

  os.remove(os.path.join(patchfile.ramdiskdir, "ramdisks.tar"))

if not ramdisks:
  print("No ramdisks found")
  sys.exit(1)

ramdisks.sort()

if list_ramdisks:
  for i in ramdisks:
    print(i)
  sys.exit(0)

print("Replacing ramdisk in: " + filename)
print("")
print("Which ramdisk do you want to use?")
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
