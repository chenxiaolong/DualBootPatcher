#!/usr/bin/env python3

# For Qualcomm based Samsung Galaxy S4 only!

import os
import re
import shutil
import subprocess
import sys
import tempfile
import zipfile

ramdisk_offset  = "0x02000000"
rootdir         = os.path.dirname(os.path.realpath(__file__))
rootdir         = os.path.join(rootdir, "..")
ramdiskdir      = os.path.join(rootdir, "ramdisks")
patchdir        = os.path.join(rootdir, "patches")
binariesdir     = os.path.join(rootdir, "binaries")
remove_dirs     = []

mkbootimg       = "mkbootimg"
unpackbootimg   = "unpackbootimg"
if os.name == "nt":
  mkbootimg     = "mkbootimg.exe"
  unpackbootimg = "unpackbootimg.exe"

def clean_up_and_exit(exit_status):
  for d in remove_dirs:
    shutil.rmtree(d)
  sys.exit(exit_status)

def read_file_one_line(filepath):
  f = open(filepath, 'r')
  line = f.readline()
  f.close()
  return line.strip('\n')

def run_command(command):
  try:
    process = subprocess.Popen(
      command,
      stdout = subprocess.PIPE,
      universal_newlines = True
    )
    output = process.communicate()[0]
    exit_status = process.returncode
    return (exit_status, output)
  except:
    print("Failed to run command: \"%s\"" % ' '.join(command))
    clean_up_and_exit(1)

def apply_patch_file(patchfile, directory):
  patch = "patch"
  if os.name == "nt":
    # Windows wants anything named patch.exe to run as Administrator
    patch = os.path.join(binariesdir, "hctap.exe")

  exit_status, output = run_command(
    [ patch,
      '-p', '1',
      '-d', directory,
      '-i', os.path.join(patchdir, patchfile)]
  )
  if exit_status != 0:
    print("Failed to apply patch")
    clean_up_and_exit(1)

def patch_boot_image(boot_image, vendor):
  tempdir = tempfile.mkdtemp()
  remove_dirs.append(tempdir)

  exit_status, output = run_command(
    [ os.path.join(binariesdir, unpackbootimg),
      '-i', boot_image,
      '-o', tempdir]
  )
  if exit_status != 0:
    print("Failed to extract boot image")
    clean_up_and_exit(1)

  prefix   = os.path.join(tempdir, os.path.split(boot_image)[1])
  base     = "0x" + read_file_one_line(prefix + "-base")
  cmdline  =        read_file_one_line(prefix + "-cmdline")
  pagesize =        read_file_one_line(prefix + "-pagesize")
  os.remove(prefix + "-base")
  os.remove(prefix + "-cmdline")
  os.remove(prefix + "-pagesize")
  os.remove(prefix + "-ramdisk.gz")
  shutil.move(prefix + "-zImage", os.path.join(tempdir, "kernel.img"))

  if vendor == "ktoonsez":
    print("Using patched ktoonsez ramdisk")
    ramdisk = "ktoonsez.dualboot.cpio.gz"

  elif vendor == "faux":
    print("Using patched Cyanogenmod ramdisk (compatible with faux)")
    ramdisk = "cyanogenmod.dualboot.cpio.gz"

  elif vendor == "pacman":
    print("Using patched Cyanogenmod ramdisk (compatible with PAC-Man)")
    ramdisk = "cyanogenmod.dualboot.cpio.gz"

  elif vendor == "chronickernel":
    print("Using patched ChronicKernel ramdisk")
    ramdisk = "chronickernel.dualboot.cpio.gz"

  else:
    print("Using patched Cyanogenmod ramdisk")
    ramdisk = "cyanogenmod.dualboot.cpio.gz"

  ramdisk = os.path.join(ramdiskdir, ramdisk)
  shutil.copyfile(ramdisk, os.path.join(tempdir, "ramdisk.cpio.gz"))

  exit_status, output = run_command(
    [ os.path.join(binariesdir, mkbootimg),
      '--kernel',         os.path.join(tempdir, "kernel.img"),
      '--ramdisk',        os.path.join(tempdir, "ramdisk.cpio.gz"),
      '--cmdline',        cmdline,
      '--base',           base,
      '--pagesize',       pagesize,
      '--ramdisk_offset', ramdisk_offset,
      '--output',         os.path.join(tempdir, "complete.img")]
  )

  os.remove(os.path.join(tempdir, "kernel.img"))
  os.remove(os.path.join(tempdir, "ramdisk.cpio.gz"))

  if exit_status != 0:
    print("Failed to create boot image")
    clean_up_and_exit(1)

  return os.path.join(tempdir, "complete.img")

def patch_zip(zip_file, vendor):
  print("--- Please wait. This may take a while ---")

  tempdir = tempfile.mkdtemp()
  remove_dirs.append(tempdir)

  z = zipfile.ZipFile(zip_file, "r")
  z.extractall(tempdir)
  z.close()

  patch_file = ""
  has_boot_image = True

  if vendor == "ktoonsez":
    patch_file = "ktoonsez.dualboot.patch"

  elif vendor == "faux":
    patch_file = "faux.dualboot.patch"

  elif vendor == "chronickernel":
    patch_file = "chronickernel.dualboot.patch"

  elif vendor == "cyanogenmod" or \
       vendor == "pacman":
    patch_file = "cyanogenmod.dualboot.patch"

  elif vendor == "cyanogenmod-gapps":
    patch_file = "cyanogenmod-gapps.dualboot.patch"
    has_boot_image = False

  if has_boot_image:
    boot_image = os.path.join(tempdir, "boot.img")
    new_boot_image = patch_boot_image(boot_image, vendor)

    os.remove(boot_image)
    shutil.move(new_boot_image, boot_image)
  else:
    print("No boot image to patch")

  shutil.copy(os.path.join(patchdir, "dualboot.sh"), tempdir)

  if patch_file != "":
    apply_patch_file(patch_file, tempdir)

  new_zip_file = os.path.join(tempdir, "complete.zip")
  z = zipfile.ZipFile(new_zip_file, 'w', zipfile.ZIP_DEFLATED)
  for root, dirs, files in os.walk(tempdir):
    for f in files:
      if f == "complete.zip":
        continue
      arcdir = os.path.relpath(root, start = tempdir)
      z.write(os.path.join(root, f), arcname = os.path.join(arcdir, f))
  z.close()

  return new_zip_file

def detect_vendor(path):
  filename = os.path.split(path)[1]

  # Custom kernels
  if re.search(r"^KT-SGS4-JB4.3-AOSP-.*.zip$", filename):
    print("Detected ktoonsez kernel zip")
    return "ktoonsez"

  elif re.search(r"^jflte[a-z]+-aosp-faux123-.*.zip$", filename):
    print("Detected faux kernel zip")
    return "faux"

  elif re.search(r"^ChronicKernel-JB4.3-AOSP-.*.zip$", filename):
    print("Detected ChronicKernel kernel zip")
    return "chronickernel"

  # Cyanogenmod ROMs
  elif re.search(r"^cm-[0-9\.]+-[0-9]+-NIGHTLY-[a-z0-9]+.zip$", filename):
    print("Detected official Cyanogenmod nightly ROM zip")
    return "cyanogenmod"

  elif re.search(r"^cm-[0-9\.]+-[0-9]+-.*.zip$", filename):
    # ROMs that have built in dual-boot support
    if "noobdev" in filename:
      print("ROM has built in dual boot support")
      clean_up_and_exit(1)

    print("Detected Cyanogenmod based ROM zip")
    return "cyanogenmod"

  # PAC-Man ROMs
  elif re.search(r"^pac_[a-z0-9]+_.*.zip$", filename):
    print("Detected PAC-Man ROM zip")
    return "pacman"

  elif re.search(r"^pac_[a-z0-9]+-nightly-[0-9]+.zip$", filename):
    print("Detected PAC-Man nightly ROM zip")
    return "pacman"

  # Google Apps
  elif re.search(r"^gapps-jb-[0-9]{8}-signed.zip", filename):
    print("Detected Cyanogenmod Google Apps zip")
    return "cyanogenmod-gapps"

  else:
    return "UNKNOWN"

def detect_file_type(path):
  if path.endswith(".zip"):
    return "zip"
  elif path.endswith(".img"):
    return "img"
  else:
    return "UNKNOWN"

if len(sys.argv) < 2:
  print("Usage: %s [kernel file]" % sys.argv[0])
  clean_up_and_exit(1)

filename = sys.argv[1]

if not os.path.exists(filename):
  print("%s does not exist!" % filename)
  clean_up_and_exit(1)

filename = os.path.abspath(filename)
filetype = detect_file_type(filename)
filevendor = detect_vendor(filename)

if filetype == "UNKNOWN":
  print("Unsupported file")
  clean_up_and_exit(1)

if filetype == "zip":
  if filevendor == "UNKNOWN":
    print("Unsupported kernel zip")
    clean_up_and_exit(1)

  newfile = patch_zip(filename, filevendor)
  print("Successfully patched zip")
  newpath = re.sub(r"\.zip$", "_dualboot.zip", filename)
  shutil.move(newfile, newpath)
  print("Path: " + newpath)

elif filetype == "img":
  newfile = patch_boot_image(filename, filevendor)
  print("Successfully patched boot image")
  newpath = re.sub(r"\.img$", "_dualboot.img", filename)
  shutil.move(newfile, newpath)
  print("Path: " + newpath)

clean_up_and_exit(0)
