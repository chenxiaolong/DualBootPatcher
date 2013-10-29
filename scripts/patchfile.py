#!/usr/bin/env python3

# For Qualcomm based Samsung Galaxy S4 only!

# Python 2 compatibility
from __future__ import print_function

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
android         = False

if os.name == "posix":
  if platform.system() == "Linux":
    # Android
    if 'BOOTCLASSPATH' in os.environ:
      android       = True
      binariesdir   = os.path.join(binariesdir, "android")
      mkbootimg     = os.path.join(binariesdir, "mkbootimg")
      unpackbootimg = os.path.join(binariesdir, "unpackbootimg")
      patch         = os.path.join(binariesdir, "patch")
      cpio          = os.path.join(binariesdir, "cpio")
      patch         = os.path.realpath(patch)
    # Desktop Linux
    else:
      binariesdir   = os.path.join(binariesdir, "linux")
      mkbootimg     = os.path.join(binariesdir, "mkbootimg")
      unpackbootimg = os.path.join(binariesdir, "unpackbootimg")
      patch         = "patch"
      cpio          = "cpio"
  elif platform.system() == "Darwin":
    binariesdir   = os.path.join(binariesdir, "osx")
    mkbootimg     = os.path.join(binariesdir, "mkbootimg")
    unpackbootimg = os.path.join(binariesdir, "unpackbootimg")
    patch         = "patch"
    cpio          = "cpio"
  else:
    print("Unsupported posix system")
    sys.exit(1)
elif os.name == "nt":
  binariesdir   = os.path.join(binariesdir, "windows")
  mkbootimg     = os.path.join(binariesdir, "mkbootimg.exe")
  unpackbootimg = os.path.join(binariesdir, "unpackbootimg.exe")
  # Windows wants anything named patch.exe to run as Administrator
  patch         = os.path.join(binariesdir, "hctap.exe")
  cpio          = os.path.join(binariesdir, "cpio.exe")
  # Windows really sucks
  bash          = os.path.join(binariesdir, "bash.exe")

else:
  print("Unsupported operating system")
  sys.exit(1)

## Print functions ##

# Information
def print_i(msg):
  print(msg)

# Errors
def print_e(msg):
  print(msg, file = sys.stderr)

# Debug
def print_d(msg):
  # Send to stderr on Android, don't show on PC
  if android:
    print(msg, file = sys.stderr)

def print_same_line(line):
  global last_line_length
  print('\r' + (' ' * last_line_length), end="")
  last_line_length = len(line)
  print('\r' + line, end="")

def exit_with(line, fail = False):
  if android:
    if fail:
      print_e("EXITFAIL:" + line)
    else:
      print_e("EXITSUCCESS:" + line)
  else:
    print_e(line)

def status(line):
  # On Android, we're reading the output from a GUI and carriage returns mess
  # that up
  if android:
    print(line)
  else:
    print_same_line(line)

# Print command output
def print_error(output = "", error = ""):
  print("--- ERROR BEGIN ---")

  if (output):
    print("--- STDOUT BEGIN ---")
    print(output)
    print("--- STDOUT END ---")
  else:
    print("No stdout output")

  if (error):
    print("--- STDERR BEGIN ---")
    print(error)
    print("--- STDERR END ---")
  else:
    print("No stderr output")

  print("--- ERROR END ---")

## Miscellaneous ##

def clean_up_and_exit(exit_status):
  for d in remove_dirs:
    shutil.rmtree(d)
  sys.exit(exit_status)

def read_file_one_line(filepath):
  f = open(filepath, 'r')
  line = f.readline()
  f.close()
  return line.strip('\n')

last_line_length = 0

## Actions ##

def run_command(command, \
                stdin_data = None, \
                cwd = None,
                universal_newlines = True):
  print_d("Running command: " + str(command))

  try:
    process = subprocess.Popen(
      command,
      stdin = subprocess.PIPE,
      stdout = subprocess.PIPE,
      stderr = subprocess.PIPE,
      cwd = cwd,
      universal_newlines = universal_newlines
    )
    output, error = process.communicate(input = stdin_data)

    exit_status = process.returncode
    return (exit_status, output, error)
  except:
    exit_with("Failed to run command: \"%s\"" % ' '.join(command), fail = True)
    clean_up_and_exit(1)

def apply_patch_file(patchfile, directory):
  print_d("Applying patch: %s in directory %s" % (patchfile, directory))

  exit_status, output, error = run_command(
    [ patch,
      '--no-backup-if-mismatch',
      '-p', '1',
      '-d', directory,
      '-i', os.path.join(patchdir, patchfile)]
  )

  if exit_status != 0:
    print_error(output = output, error = error)
    exit_with("Failed to apply patch", fail = True)
    clean_up_and_exit(1)

def process_ramdisk_def(patch_file, directory):
  print_d("Loading ramdisk definition %s in directory %s" % (patch_file, directory))
  with open(patch_file) as f:
    for line in f.readlines():
      if line.startswith("pyscript"):
        path = os.path.join(ramdiskdir, \
                            re.search(r"^pyscript\s*=\s*\"?(.*)\"?\s*$", line).group(1))

        print_d("Loading pyscript " + path)

        plugin = imp.load_source(os.path.basename(path)[:-3], \
                                 os.path.join(ramdiskdir, path))

        plugin.patch_ramdisk(directory)

      elif line.startswith("patch"):
        path = os.path.join(ramdiskdir, \
                            re.search(r"^patch\s*=\s*\"?(.*)\"?\s*$", line).group(1))
        apply_patch_file(path, directory)

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
      print_d("Found in patch %s: %s" % (patch_file, temp.group(1)))
      files.append(temp.group(1))
      counter += 3
    else:
      counter += 1

  if not files:
    exit_with("Failed to read list of files in patch", fail = True)
    clean_up_and_exit(1)

  return files

def patch_boot_image(boot_image, file_info):
  tempdir = tempfile.mkdtemp()
  remove_dirs.append(tempdir)

  exit_status, output, error = run_command(
    [ unpackbootimg,
      '-i', boot_image,
      '-o', tempdir]
  )
  if exit_status != 0:
    print_error(output = output, error = error)
    exit_with("Failed to extract boot image", fail = True)
    clean_up_and_exit(1)

  prefix   = os.path.join(tempdir, os.path.split(boot_image)[1])
  base     = "0x" + read_file_one_line(prefix + "-base")
  cmdline  =        read_file_one_line(prefix + "-cmdline")
  pagesize =        read_file_one_line(prefix + "-pagesize")
  os.remove(prefix + "-base")
  os.remove(prefix + "-cmdline")
  os.remove(prefix + "-pagesize")

  ramdisk = os.path.join(tempdir, "ramdisk.cpio")
  kernel = os.path.join(tempdir, "kernel.img")

  shutil.move(prefix + "-zImage", kernel)

  extracted = os.path.join(tempdir, "extracted")
  os.mkdir(extracted)

  # Decompress ramdisk
  status("Extracting ramdisk")

  with gzip.open(prefix + "-ramdisk.gz", 'rb') as f_in:
    exit_status, output, error = run_command(
      [ cpio, '-i', '-d', '-m', '-v' ],
      stdin_data = f_in.read(),
      cwd = extracted,
      universal_newlines = False
    )
    if exit_status != 0:
      print_error(output = output, error = error)
      exit_with("Failed to extract ramdisk using cpio", fail = True)
      clean_up_and_exit(1)

  os.remove(prefix + "-ramdisk.gz")

  # Patch ramdisk
  if not file_info.ramdisk:
    print_i("No ramdisk patch specified")
    return None

  process_ramdisk_def(
    os.path.join(ramdiskdir, file_info.ramdisk),
    extracted
  )

  # Copy init.dualboot.mounting.sh
  shutil.copy(
    os.path.join(ramdiskdir, "init.dualboot.mounting.sh"),
    extracted
  )

  # Copy busybox
  shutil.copy(
    os.path.join(ramdiskdir, "busybox-static"),
    os.path.join(extracted, "sbin")
  )

  # Copy new init if needed
  if file_info.need_new_init:
    shutil.copy(
      os.path.join(ramdiskdir, "init"),
      extracted
    )

  # Create gzip compressed ramdisk
  ramdisk_files = []
  for root, dirs, files in os.walk(extracted):
    for d in dirs:
      if os.path.samefile(root, extracted):
        # cpio, for whatever reason, creates a directory called '.'

        # Windows sucks
        if os.name == "nt":
          ramdisk_files.append(d.replace('\\', '/'))
        else:
          ramdisk_files.append(d)

        status("Adding directory to ramdisk: %s" % d)
      else:
        relative_dir = os.path.relpath(root, extracted)

        # Windows sucks
        if os.name == "nt":
          ramdisk_files.append(os.path.join(relative_dir, d).replace('\\', '/'))
        else:
          ramdisk_files.append(os.path.join(relative_dir, d))

        status("Adding directory to ramdisk: %s" % os.path.join(relative_dir, d))

    for f in files:
      if os.path.samefile(root, extracted):
        # cpio, for whatever reason, creates a directory called '.'

        # Windows sucks
        if os.name == "nt":
          ramdisk_files.append(f.replace('\\', '/'))
        else:
          ramdisk_files.append(f)

        status("Adding file to ramdisk: %s" % f)
      else:
        relative_dir = os.path.relpath(root, extracted)

        # Windows sucks
        if os.name == "nt":
          ramdisk_files.append(os.path.join(relative_dir, f).replace('\\', '/'))
        else:
          ramdisk_files.append(os.path.join(relative_dir, f))

        status("Adding file to ramdisk: %s" % os.path.join(relative_dir, f))

  status("Creating gzip compressed ramdisk with cpio")

  ramdisk = ramdisk + ".gz"

  with gzip.open(ramdisk, 'wb') as f_out:
    if os.name == "nt":
      # We need the /cygdrive/c/Users/.../ path instead of C:\Users\...\ because
      # a backslash is a valid character in a Unix path. When the list of files
      # is passed to cpio, sbin/adbd, for example, would be included as a file
      # named 'sbin\adbd'
      stdin = "cd '%s' && pwd\n" % binariesdir

      exit_status, output, error = run_command(
        [ bash ],
        stdin_data = stdin.encode("UTF-8"),
        cwd = extracted,
        universal_newlines = False
      )
      if exit_status != 0:
        print_error(output = output, error = error)
        exit_with("Failed to get Cygwin drive path", fail = True)
        clean_up_and_exit(1)

      cpio_cygpath = output.decode("UTF-8").strip('\n') + '/cpio.exe'

      stdin = output.decode("UTF-8").strip('\n') + '/cpio.exe' + \
              " -o -H newc <<EOF\n"                            + \
              '\n'.join(ramdisk_files) + "\n"                  + \
              "EOF\n"

      # We cannot use "bash -c '...'" because the /cygdrive/ mountpoints are
      # created only in an interactive shell. We'll launch bash first and then
      # run cpio.
      exit_status, output, error = run_command(
        [ bash ],
        stdin_data = stdin.encode("UTF-8"),
        cwd = extracted,
        universal_newlines = False
      )

    else:
      # So much easier than in Windows...
      exit_status, output, error = run_command(
        [ cpio, '-o', '-H', 'newc' ],
        stdin_data = '\n'.join(ramdisk_files).encode("UTF-8"),
        cwd = extracted,
        universal_newlines = False
      )

    if exit_status != 0:
      print_error(output = output, error = error)
      exit_with("Failed to create gzip compressed ramdisk", fail = True)
      clean_up_and_exit(1)

    f_out.write(output)

  status("Creating boot image")

  exit_status, output, error = run_command(
    [ mkbootimg,
      '--kernel',         kernel,
      '--ramdisk',        ramdisk,
      '--cmdline',        cmdline,
      '--base',           base,
      '--pagesize',       pagesize,
      '--ramdisk_offset', ramdisk_offset,
      '--output',         os.path.join(tempdir, "complete.img")]
  )

  os.remove(kernel)
  os.remove(ramdisk)

  if exit_status != 0:
    print_error(output = output, error = error)
    exit_with("Failed to create boot image", fail = True)
    clean_up_and_exit(1)

  return os.path.join(tempdir, "complete.img")

def patch_zip(zip_file, file_info):
  if not android:
    print_i("--- Please wait. This may take a while ---")

  files_to_patch = ""
  if file_info.patch != "":
    files_to_patch = files_in_patch(file_info.patch)

  tempdir = tempfile.mkdtemp()
  remove_dirs.append(tempdir)

  status("Loading zip file")
  z = zipfile.ZipFile(zip_file, "r")
  for f in files_to_patch:
    status("Extracting file to be patched: %s" % f)
    z.extract(f, path = tempdir)
  if file_info.has_boot_image:
    status("Extracting boot image: %s" % file_info.bootimg)
    z.extract(file_info.bootimg, path = tempdir)
  z.close()

  if not android:
    status("")

  if file_info.has_boot_image:
    boot_image = os.path.join(tempdir, file_info.bootimg)
    new_boot_image = patch_boot_image(boot_image, file_info)

    os.remove(boot_image)
    shutil.move(new_boot_image, boot_image)
  elif file_info.loki:
    print_i("*** IMPORTANT: This zip contains a loki'd kernel which canoot be patched. You MUST patch and flash a custom kernel to boot ***")
  else:
    print_i("No boot image to patch")

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

    status("Adding file to zip: %s" % i.filename)
    z_output.writestr(i.filename, z_input.read(i.filename))

  if not android:
    status("")

  z_input.close()

  for root, dirs, files in os.walk(tempdir):
    for f in files:
      if f == "complete.zip":
        continue
      status("Adding file to zip: %s" % f)
      arcdir = os.path.relpath(root, start = tempdir)
      z_output.write(os.path.join(root, f), arcname = os.path.join(arcdir, f))

  if not android:
    status("")

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
            print_d("Loading patchinfo plugin: " + filename)
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
    print_i("Usage: %s [zip file or boot.img]" % sys.argv[0])
    clean_up_and_exit(1)

  filename = sys.argv[1]

  if not os.path.exists(filename):
    print_i("%s does not exist!" % filename)
    clean_up_and_exit(1)

  filename = os.path.abspath(filename)
  filetype = detect_file_type(filename)
  fileinfo = get_file_info(filename)

  if filetype == "UNKNOWN":
    exit_with("Unsupported file", fail = True)
    clean_up_and_exit(1)

  if filetype == "zip":
    if not fileinfo:
      exit_with("Unsupported zip", fail = True)
      clean_up_and_exit(1)

    # Patch zip and get path to patched zip
    newfile = patch_zip(filename, fileinfo)

    exit_with("Successfully patched zip")

    newpath = re.sub(r"\.zip$", "_dualboot.zip", filename)
    shutil.copyfile(newfile, newpath)
    os.remove(newfile)

    if not android:
      print_i("Path: " + newpath)

  elif filetype == "img":
    newfile = patch_boot_image(filename, fileinfo)

    exit_with("Successfully patched boot image")

    newpath = re.sub(r"\.img$", "_dualboot.img", filename)
    shutil.copyfile(newfile, newpath)
    os.remove(newfile)

    if not android:
      print_i("Path: " + newpath)

  clean_up_and_exit(0)