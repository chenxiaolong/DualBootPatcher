#!/usr/bin/env python3

import os
import re
import shutil

def write(f, line):
  f.write(line.encode("UTF-8"))

def whitespace(line):
  m = re.search(r"^(\s+).*$", line)
  if m and m.group(1):
    return m.group(1)
  else:
    return ""

def modify_init_rc(directory):
  f = open(os.path.join(directory, 'init.rc'))
  lines = f.readlines()
  f.close()

  f = open(os.path.join(directory, 'init.rc'), 'wb')
  for line in lines:
    if 'export ANDROID_ROOT' in line:
      write(f, line)
      write(f, whitespace(line) + "export ANDROID_CACHE /cache\n")

    elif re.search(r"mkdir /system(\s|$)", line):
      write(f, line)
      write(f, re.sub("/system", "/raw-system", line))

    elif re.search(r"mkdir /data(\s|$)", line):
      write(f, line)
      write(f, re.sub("/data", "/raw-data", line))

    elif re.search(r"mkdir /cache(\s|$)", line):
      write(f, line)
      write(f, re.sub("/cache", "/raw-cache", line))

    elif 'yaffs2' in line:
      write(f, re.sub(r"^", "#", line))

    else:
      write(f, line)

  f.close()

def modify_init_qcom_rc(directory):
  f = open(os.path.join(directory, 'init.qcom.rc'))
  lines = f.readlines()
  f.close()

  f = open(os.path.join(directory, 'init.qcom.rc'), 'wb')
  for line in lines:
    if 'export EMULATED_STORAGE_TARGET' in line:
      write(f, line)
      write(f, whitespace(line) + "export EXTERNAL_SD /storage/sdcard1\n")

    # Change /data/media to /raw-data/media
    elif re.search(r"/data/media(\s|$)", line):
      write(f, re.sub('/data/media', '/raw-data/media', line))

    else:
      write(f, line)

  f.close()

def modify_fstab(directory):
  f = open(os.path.join(directory, 'fstab.qcom'))
  lines = f.readlines()
  f.close()

  # For Android 4.2 ROMs
  has_cache_line = False

  f = open(os.path.join(directory, 'fstab.qcom'), 'wb')
  for line in lines:
    if re.search(r"^/dev[a-zA-Z0-9/\._-]+\s+/system\s+.*$", line):
      write(f, re.sub("\s/system\s", " /raw-system ", line))

    elif re.search(r"^/dev[^\s]+\s+/cache\s+.*$", line):
      write(f, re.sub("\s/cache\s", " /raw-cache ", line))
      has_cache_line = True

    elif re.search(r"^/dev[^\s]+\s+/data\s+.*$", line):
      write(f, re.sub("\s/data\s", " /raw-data ", line))

    else:
      write(f, line)

  if not has_cache_line:
    write(f, "/dev/block/platform/msm_sdcc.1/by-name/cache          /raw-cache       ext4    nosuid,nodev,barrier=1 wait,check")

  f.close()

def patch_ramdisk(directory):
  modify_init_rc(directory)
  modify_init_qcom_rc(directory)
  modify_fstab(directory)
