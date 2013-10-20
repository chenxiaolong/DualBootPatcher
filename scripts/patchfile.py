#!/usr/bin/env python3

# For Qualcomm based Samsung Galaxy S4 only!

import gzip
import imp
import os
import platform
import re
import shutil
import subprocess
import sys
import tarfile
import tempfile
import zipfile

ramdisk_offset  = "0x02000000"
rootdir         = os.path.dirname(os.path.realpath(__file__))
rootdir         = os.path.join(rootdir, "..")
ramdiskdir      = os.path.join(rootdir, "ramdisks")
patchdir        = os.path.join(rootdir, "patches")
binariesdir     = os.path.join(rootdir, "binaries")
patchinfodir    = os.path.join(rootdir, "patchinfo")
remove_dirs     = []

if os.name == "posix":
  if platform.system() == "Linux":
    binariesdir   = os.path.join(binariesdir, "linux")
    mkbootimg     = os.path.join(binariesdir, "mkbootimg")
    unpackbootimg = os.path.join(binariesdir, "unpackbootimg")
    xz            = "xz"
    patch         = "patch"
  elif platform.system() == "Darwin":
    binariesdir   = os.path.join(binariesdir, "osx")
    mkbootimg     = os.path.join(binariesdir, "mkbootimg")
    unpackbootimg = os.path.join(binariesdir, "unpackbootimg")
    xz            = os.path.join(binariesdir, "bin/xz")
    patch         = "patch"
  else:
    print("Unsupported posix system")
    sys.exit(1)
elif os.name == "nt":
  binariesdir   = os.path.join(binariesdir, "windows")
  mkbootimg     = os.path.join(binariesdir, "mkbootimg.exe")
  unpackbootimg = os.path.join(binariesdir, "unpackbootimg.exe")
  xz            = os.path.join(binariesdir, "xz.exe")
  # Windows wants anything named patch.exe to run as Administrator
  patch         = os.path.join(binariesdir, "hctap.exe")

else:
  print("Unsupported operating system")
  sys.exit(1)

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
  exit_status, output = run_command(
    [ patch,
      '-p', '1',
      '-d', directory,
      '-i', os.path.join(patchdir, patchfile)]
  )
  if exit_status != 0:
    print("Failed to apply patch")
    clean_up_and_exit(1)

def files_in_patch(patch_file):
  files = []

  f = open(os.path.join(patchdir, patch_file))
  lines = f.readlines()
  f.close()

  counter = 0
  while counter < len(lines):
    if re.search(r"^---",    lines[counter])     and \
       re.search(r"^\+\+\+", lines[counter + 1]) and \
       re.search(r"^@",      lines[counter + 2]):
      temp = re.search(r"^--- .*?/(.*)$", lines[counter])
      files.append(temp.group(1))
      counter += 3
    else:
      counter += 1

  if not files:
    print("Failed to read list of files in patch.")
    clean_up_and_exit(1)

  return files

def patch_boot_image(boot_image, file_info):
  tempdir = tempfile.mkdtemp()
  remove_dirs.append(tempdir)

  exit_status, output = run_command(
    [ unpackbootimg,
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

  if not file_info.ramdisk:
    print("No ramdisk specified")
    return None

  # Extract ramdisk from tar.xz
  if sys.hexversion >= 50528256: # Python 3.3
    with tarfile.open(os.path.join(ramdiskdir, "ramdisks.tar.xz")) as f:
      f.extract(file_info.ramdisk, path = ramdiskdir)
  else:
    run_command(
      [ xz, '-d', '-k', '-f', os.path.join(ramdiskdir, "ramdisks.tar.xz") ]
    )

    with tarfile.open(os.path.join(ramdiskdir, "ramdisks.tar")) as f:
      f.extract(file_info.ramdisk, path = ramdiskdir)

    os.remove(os.path.join(ramdiskdir, "ramdisks.tar"))

  ramdisk = os.path.join(ramdiskdir, file_info.ramdisk)

  # Compress ramdisk with gzip if it isn't already
  if not file_info.ramdisk.endswith(".gz"):
    with open(ramdisk, 'rb') as f_in:
      with gzip.open(ramdisk + ".gz", 'wb') as f_out:
        f_out.writelines(f_in)

    os.remove(os.path.join(ramdiskdir, file_info.ramdisk))
    ramdisk = ramdisk + ".gz"

  shutil.copyfile(ramdisk, os.path.join(tempdir, "ramdisk.cpio.gz"))

  os.remove(os.path.join(ramdiskdir, file_info.ramdisk + ".gz"))

  exit_status, output = run_command(
    [ mkbootimg,
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

def patch_zip(zip_file, file_info):
  print("--- Please wait. This may take a while ---")

  files_to_patch = ""
  if file_info.patch != "":
    files_to_patch = files_in_patch(file_info.patch)

  tempdir = tempfile.mkdtemp()
  remove_dirs.append(tempdir)

  z = zipfile.ZipFile(zip_file, "r")
  for f in files_to_patch:
    z.extract(f, path = tempdir)
  if file_info.has_boot_image:
    z.extract(file_info.bootimg, path = tempdir)
  z.close()

  if file_info.has_boot_image:
    boot_image = os.path.join(tempdir, file_info.bootimg)
    new_boot_image = patch_boot_image(boot_image, file_info)

    os.remove(boot_image)
    shutil.move(new_boot_image, boot_image)
  elif file_info.loki:
    print("*** IMPORTANT: This zip contains a loki'd kernel which canoot be patched. You MUST patch and flash a custom kernel to boot ***")
  else:
    print("No boot image to patch")

  shutil.copy(os.path.join(patchdir, "dualboot.sh"), tempdir)

  if file_info.patch != "":
    apply_patch_file(file_info.patch, tempdir)

  # We can't avoid recompression, unfortunately
  new_zip_file = os.path.join(tempdir, "complete.zip")
  z_input = zipfile.ZipFile(zip_file, 'r')
  z_output = zipfile.ZipFile(new_zip_file, 'w', zipfile.ZIP_DEFLATED)

  for i in z_input.infolist():
    # Skip patched files
    if i.filename in files_to_patch:
      continue
    # Boot image too
    elif i.filename == file_info.bootimg:
      continue
    z_output.writestr(i.filename, z_input.read(i.filename))

  z_input.close()

  for root, dirs, files in os.walk(tempdir):
    for f in files:
      if f == "complete.zip":
        continue
      arcdir = os.path.relpath(root, start = tempdir)
      z_output.write(os.path.join(root, f), arcname = os.path.join(arcdir, f))

  z_output.close()

  return new_zip_file

def get_file_info(path):
  filename = os.path.split(path)[1]

  # TODO: Don't hardcode this
  for i in [ 'Google_Apps', 'Other', 'jflte' ]:
    for root, dirs, files in os.walk(os.path.join(patchinfodir, i)):
      for f in files:
        if f.endswith(".py"):
          plugin = imp.load_source(os.path.basename(f)[:-3], \
                                   os.path.join(root, f))
          if plugin.matches(filename):
            plugin.print_message()
            return plugin.get_file_info()

  return None

def detect_file_type(path):
  if path.endswith(".zip"):
    return "zip"
  elif path.endswith(".img"):
    return "img"
  else:
    return "UNKNOWN"

if __name__ == "__main__":
  if len(sys.argv) < 2:
    print("Usage: %s [zip file or boot.img]" % sys.argv[0])
    clean_up_and_exit(1)

  filename = sys.argv[1]

  if not os.path.exists(filename):
    print("%s does not exist!" % filename)
    clean_up_and_exit(1)

  filename = os.path.abspath(filename)
  filetype = detect_file_type(filename)
  fileinfo = get_file_info(filename)

  if filetype == "UNKNOWN":
    print("Unsupported file")
    clean_up_and_exit(1)

  if filetype == "zip":
    if not fileinfo:
      print("Unsupported zip")
      clean_up_and_exit(1)

    newfile = patch_zip(filename, fileinfo)
    print("Successfully patched zip")
    newpath = re.sub(r"\.zip$", "_dualboot.zip", filename)
    shutil.move(newfile, newpath)
    print("Path: " + newpath)

  elif filetype == "img":
    newfile = patch_boot_image(filename, fileinfo)
    print("Successfully patched boot image")
    newpath = re.sub(r"\.img$", "_dualboot.img", filename)
    shutil.move(newfile, newpath)
    print("Path: " + newpath)

  clean_up_and_exit(0)
