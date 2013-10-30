#!/usr/bin/env python3

import os
import re
import shutil
import sys

def write(f, line):
  if sys.hexversion < 0x03030000:
    f.write(line)
  else:
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
  for i in [ 'init.qcom.rc', 'init.jgedlte.rc' ]:
    f = open(os.path.join(directory, i))
    lines = f.readlines()
    f.close()

    f = open(os.path.join(directory, i), 'wb')
    for line in lines:
      # Change /data/media to /raw-data/media
      if re.search(r"/data/media(\s|$)", line):
        write(f, re.sub('/data/media', '/raw-data/media', line))

      else:
        write(f, line)

    f.close()

def modify_fstab(directory):
  # Ignore all contents for Google Edition
  for i in [ 'fstab.qcom', 'fstab.jgedlte' ]:
    f = open(os.path.join(directory, i))
    lines = f.readlines()
    f.close()

    has_cache_line = False

    f = open(os.path.join(directory, i), 'wb')
    for line in lines:
      if re.search(r"^/dev[a-zA-Z0-9/\._-]+\s+/system\s+.*$", line):
        write(f, "/dev/block/platform/msm_sdcc.1/by-name/system /raw-system ext4 ro,errors=panic wait\n")

      elif re.search(r"^/dev[^\s]+\s+/cache\s+.*$", line):
        write(f, "/dev/block/platform/msm_sdcc.1/by-name/cache /raw-cache ext4 nosuid,nodev,barrier=1 wait,check\n")
        has_cache_line = True

      elif re.search(r"^/dev[^\s]+\s+/data\s+.*$", line):
        write(f, "/dev/block/platform/msm_sdcc.1/by-name/userdata /raw-data ext4 nosuid,nodev,noatime,noauto_da_alloc,discard,journal_async_commit,errors=panic wait,check,encryptable=footer\n")

      else:
        write(f, line)

    if not has_cache_line:
      write(f, "/dev/block/platform/msm_sdcc.1/by-name/cache /raw-cache ext4 nosuid,nodev,barrier=1 wait,check\n")

    f.close()

def modify_init_target_rc(directory):
  f = open(os.path.join(directory, 'init.target.rc'))
  lines = f.readlines()
  f.close()

  previous_line = ""

  f = open(os.path.join(directory, 'init.target.rc'), 'wb')
  for line in lines:
    if re.search(r"^\s+wait\s+/dev/.*/cache.*$", line):
      write(f, re.sub(r"^", "#", line))

    elif re.search(r"^\s+check_fs\s+/dev/.*/cache.*$", line):
      write(f, re.sub(r"^", "#", line))

    elif re.search(r"^\s+mount\s+ext4\s+/dev/.*/cache.*$", line):
      write(f, re.sub(r"^", "#", line))

    elif re.search(r"^\s+mount_all\s+fstab.jgedlte.*$", line) and \
        re.search(r"^on\s+fs_selinux.*$", previous_line):
      write(f, line)
      write(f, whitespace(line) + "exec /sbin/busybox-static sh /init.dualboot.mounting.sh\n")

    else:
      write(f, line)

    previous_line = line

  f.close()

def modify_MSM8960_lpm_rc(directory):
  f = open(os.path.join(directory, 'MSM8960_lpm.rc'))
  lines = f.readlines()
  f.close()

  f = open(os.path.join(directory, 'MSM8960_lpm.rc'), 'wb')
  for line in lines:
    if re.search(r"^\s+mount.*/cache.*$", line):
      write(f, re.sub(r"^", "#", line))

    else:
      write(f, line)

  f.close()

def patch_ramdisk(directory):
  modify_init_rc(directory)
  modify_init_qcom_rc(directory)
  modify_fstab(directory)
  modify_init_target_rc(directory)
  modify_MSM8960_lpm_rc(directory)
