#!/usr/bin/env python3

import common as c
import os
import re
import shutil
import sys

move_apnhlos_mount = False
move_mdm_mount = False

def modify_init_rc(directory):
  lines = c.get_lines_from_file(directory, 'init.rc')

  f = c.open_file(directory, 'init.rc', c.WRITE)
  for line in lines:
    if 'export ANDROID_ROOT' in line:
      c.write(f, line)
      c.write(f, c.whitespace(line) + "export ANDROID_CACHE /cache\n")

    elif re.search(r"mkdir /system(\s|$)", line):
      c.write(f, line)
      c.write(f, re.sub("/system", "/raw-system", line))

    elif re.search(r"mkdir /data(\s|$)", line):
      c.write(f, line)
      c.write(f, re.sub("/data", "/raw-data", line))

    elif re.search(r"mkdir /cache(\s|$)", line):
      c.write(f, line)
      c.write(f, re.sub("/cache", "/raw-cache", line))

    elif 'yaffs2' in line:
      c.write(f, re.sub(r"^", "#", line))

    else:
      c.write(f, line)

  f.close()

def modify_init_qcom_rc(directory):
  lines = c.get_lines_from_file(directory, 'init.qcom.rc')

  f = c.open_file(directory, 'init.qcom.rc', c.WRITE)
  for line in lines:
    if 'export EMULATED_STORAGE_TARGET' in line:
      c.write(f, line)
      c.write(f, c.whitespace(line) + "export EXTERNAL_SD /storage/sdcard1\n")

    # Change /data/media to /raw-data/media
    elif re.search(r"/data/media(\s|$)", line):
      c.write(f, re.sub('/data/media', '/raw-data/media', line))

    else:
      c.write(f, line)

  f.close()

def modify_fstab(directory, partition_config):
  lines = c.get_lines_from_file(directory, 'fstab.qcom')

  system_fourth = 'ro,barrier=1,errors=panic'
  system_fifth = 'wait'
  cache_fourth = 'nosuid,nodev,barrier=1'
  cache_fifth = 'wait,check'

  # For Android 4.2 ROMs
  has_cache_line = False

  f = c.open_file(directory, 'fstab.qcom', c.WRITE)
  for line in lines:
    if re.search(r"^/dev[a-zA-Z0-9/\._-]+\s+/system\s+.*$", line):
      temp = re.sub("\s/system\s", " /raw-system ", line)

      if '/raw-system' in partition_config.target_cache:
        r = re.search(r"^([^\ ]+)\s+([^\ ]+)\s+([^\ ]+)\s+([^\ ]+)\s+([^\ ]+)", temp)
        temp = "%s %s %s %s %s\n" % (r.groups()[0], r.groups()[1], r.groups()[2],
                                     cache_fourth, cache_fifth)

      c.write(f, temp)

    elif re.search(r"^/dev[^\s]+\s+/cache\s+.*$", line):
      temp = re.sub("\s/cache\s", " /raw-cache ", line)
      has_cache_line = True

      if '/raw-cache' in partition_config.target_system:
        r = re.search(r"^([^\ ]+)\s+([^\ ]+)\s+([^\ ]+)\s+([^\ ]+)\s+([^\ ]+)", temp)
        temp = "%s %s %s %s %s\n" % (r.groups()[0], r.groups()[1], r.groups()[2],
                                     system_fourth, system_fifth)

      c.write(f, temp)

    elif re.search(r"^/dev[^\s]+\s+/data\s+.*$", line):
      temp = re.sub("\s/data\s", " /raw-data ", line)

      if '/raw-data' in partition_config.target_system:
        r = re.search(r"^([^\ ]+)\s+([^\ ]+)\s+([^\ ]+)\s+([^\ ]+)\s+([^\ ]+)", temp)
        temp = "%s %s %s %s %s\n" % (r.groups()[0], r.groups()[1], r.groups()[2],
                                     system_fourth, system_fifth)

      c.write(f, temp)

    elif re.search(r"^/dev/[^\s]+apnhlos\s", line):
      global move_apnhlos_mount
      move_apnhlos_mount = True
      continue

    elif re.search(r"^/dev/[^\s]+mdm\s", line):
      global move_mdm_mount
      move_mdm_mount = True
      continue

    else:
      c.write(f, line)

  if not has_cache_line:
    if '/raw-cache' in partition_config.target_system:
      c.write(f, "/dev/block/platform/msm_sdcc.1/by-name/cache /raw-cache ext4 %s %s\n" % (system_fourth, system_fifth))
    else:
      c.write(f, "/dev/block/platform/msm_sdcc.1/by-name/cache /raw-cache ext4 nosuid,nodev,barrier=1 wait,check\n")

  f.close()

def modify_init_target_rc(directory):
  lines = c.get_lines_from_file(directory, 'init.target.rc')

  previous_line = ""

  f = c.open_file(directory, 'init.target.rc', c.WRITE)
  for line in lines:
    if re.search(r"^\s+wait\s+/dev/.*/cache.*$", line):
      c.write(f, re.sub(r"^", "#", line))

    elif re.search(r"^\s+check_fs\s+/dev/.*/cache.*$", line):
      c.write(f, re.sub(r"^", "#", line))

    elif re.search(r"^\s+mount\s+ext4\s+/dev/.*/cache.*$", line):
      c.write(f, re.sub(r"^", "#", line))

    elif re.search(r"^\s+mount_all\s+fstab.qcom.*$", line) and \
        re.search(r"^on\s+fs\s*$", previous_line):
      c.write(f, line)
      c.write(f, c.whitespace(line) + "exec /sbin/busybox-static sh /init.multiboot.mounting.sh\n")

    elif re.search(r"^\s+setprop\s+ro.crypto.fuse_sdcard\s+true", line):
      c.write(f, line)

      if move_apnhlos_mount:
        c.write(f, c.whitespace(line) + "wait /dev/block/platform/msm_sdcc.1/by-name/apnhlos\n")
        c.write(f, c.whitespace(line) + "mount vfat /dev/block/platform/msm_sdcc.1/by-name/apnhlos /firmware ro shortname=lower,uid=1000,gid=1000,dmask=227,fmask=337\n")

      if move_mdm_mount:
        c.write(f, c.whitespace(line) + "wait /dev/block/platform/msm_sdcc.1/by-name/mdm\n")
        c.write(f, c.whitespace(line) + "mount vfat /dev/block/platform/msm_sdcc.1/by-name/mdm /firmware-mdm ro shortname=lower,uid=1000,gid=1000,dmask=227,fmask=337\n")

    else:
      c.write(f, line)

    previous_line = line

  f.close()

def patch_ramdisk(directory, partition_config):
  modify_init_rc(directory)
  modify_init_qcom_rc(directory)
  modify_fstab(directory, partition_config)
  modify_init_target_rc(directory)
