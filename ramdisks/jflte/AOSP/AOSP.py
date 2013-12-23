#!/usr/bin/env python3

import multiboot.fileio as fileio

import os
import re
import shutil
import sys

move_apnhlos_mount = False
move_mdm_mount = False

def modify_init_rc(directory):
  lines = fileio.all_lines('init.rc', directory = directory)

  f = fileio.open_file('init.rc', fileio.WRITE, directory = directory)
  for line in lines:
    if 'export ANDROID_ROOT' in line:
      fileio.write(f, line)
      fileio.write(f, fileio.whitespace(line) + "export ANDROID_CACHE /cache\n")

    elif re.search(r"mkdir /system(\s|$)", line):
      fileio.write(f, line)
      fileio.write(f, re.sub("/system", "/raw-system", line))

    elif re.search(r"mkdir /data(\s|$)", line):
      fileio.write(f, line)
      fileio.write(f, re.sub("/data", "/raw-data", line))

    elif re.search(r"mkdir /cache(\s|$)", line):
      fileio.write(f, line)
      fileio.write(f, re.sub("/cache", "/raw-cache", line))

    elif 'yaffs2' in line:
      fileio.write(f, re.sub(r"^", "#", line))

    else:
      fileio.write(f, line)

  f.close()

def modify_init_qcom_rc(directory):
  lines = fileio.all_lines('init.qcom.rc', directory = directory)

  f = fileio.open_file('init.qcom.rc', fileio.WRITE, directory = directory)
  for line in lines:
    if 'export EMULATED_STORAGE_TARGET' in line:
      fileio.write(f, line)
      fileio.write(f, fileio.whitespace(line) + "export EXTERNAL_SD /storage/sdcard1\n")

    # Change /data/media to /raw-data/media
    elif re.search(r"/data/media(\s|$)", line):
      fileio.write(f, re.sub('/data/media', '/raw-data/media', line))

    else:
      fileio.write(f, line)

  f.close()

def modify_fstab(directory, partition_config):
  lines = fileio.all_lines('fstab.qcom', directory = directory)

  system_fourth = 'ro,barrier=1,errors=panic'
  system_fifth = 'wait'
  cache_fourth = 'nosuid,nodev,barrier=1'
  cache_fifth = 'wait,check'

  # For Android 4.2 ROMs
  has_cache_line = False

  f = fileio.open_file('fstab.qcom', fileio.WRITE, directory = directory)
  for line in lines:
    if re.search(r"^/dev[a-zA-Z0-9/\._-]+\s+/system\s+.*$", line):
      temp = re.sub("\s/system\s", " /raw-system ", line)

      if '/raw-system' in partition_config.target_cache:
        r = re.search(r"^([^\ ]+)\s+([^\ ]+)\s+([^\ ]+)\s+([^\ ]+)\s+([^\ ]+)", temp)
        temp = "%s %s %s %s %s\n" % (r.groups()[0], r.groups()[1], r.groups()[2],
                                     cache_fourth, cache_fifth)

      fileio.write(f, temp)

    elif re.search(r"^/dev[^\s]+\s+/cache\s+.*$", line):
      temp = re.sub("\s/cache\s", " /raw-cache ", line)
      has_cache_line = True

      if '/raw-cache' in partition_config.target_system:
        r = re.search(r"^([^\ ]+)\s+([^\ ]+)\s+([^\ ]+)\s+([^\ ]+)\s+([^\ ]+)", temp)
        temp = "%s %s %s %s %s\n" % (r.groups()[0], r.groups()[1], r.groups()[2],
                                     system_fourth, system_fifth)

      fileio.write(f, temp)

    elif re.search(r"^/dev[^\s]+\s+/data\s+.*$", line):
      temp = re.sub("\s/data\s", " /raw-data ", line)

      if '/raw-data' in partition_config.target_system:
        r = re.search(r"^([^\ ]+)\s+([^\ ]+)\s+([^\ ]+)\s+([^\ ]+)\s+([^\ ]+)", temp)
        temp = "%s %s %s %s %s\n" % (r.groups()[0], r.groups()[1], r.groups()[2],
                                     system_fourth, system_fifth)

      fileio.write(f, temp)

    elif re.search(r"^/dev/[^\s]+apnhlos\s", line):
      global move_apnhlos_mount
      move_apnhlos_mount = True
      continue

    elif re.search(r"^/dev/[^\s]+mdm\s", line):
      global move_mdm_mount
      move_mdm_mount = True
      continue

    else:
      fileio.write(f, line)

  if not has_cache_line:
    if '/raw-cache' in partition_config.target_system:
      fileio.write(f, "/dev/block/platform/msm_sdcc.1/by-name/cache /raw-cache ext4 %s %s\n" % (system_fourth, system_fifth))
    else:
      fileio.write(f, "/dev/block/platform/msm_sdcc.1/by-name/cache /raw-cache ext4 nosuid,nodev,barrier=1 wait,check\n")

  f.close()

def modify_init_target_rc(directory):
  lines = fileio.all_lines('init.target.rc', directory = directory)

  previous_line = ""

  f = fileio.open_file('init.target.rc', fileio.WRITE, directory = directory)
  for line in lines:
    if re.search(r"^\s+wait\s+/dev/.*/cache.*$", line):
      fileio.write(f, re.sub(r"^", "#", line))

    elif re.search(r"^\s+check_fs\s+/dev/.*/cache.*$", line):
      fileio.write(f, re.sub(r"^", "#", line))

    elif re.search(r"^\s+mount\s+ext4\s+/dev/.*/cache.*$", line):
      fileio.write(f, re.sub(r"^", "#", line))

    elif re.search(r"^\s+mount_all\s+fstab.qcom.*$", line) and \
        re.search(r"^on\s+fs\s*$", previous_line):
      fileio.write(f, line)
      fileio.write(f, fileio.whitespace(line) + "exec /sbin/busybox-static sh /init.multiboot.mounting.sh\n")

    elif re.search(r"^\s+setprop\s+ro.crypto.fuse_sdcard\s+true", line):
      fileio.write(f, line)

      if move_apnhlos_mount:
        fileio.write(f, fileio.whitespace(line) + "wait /dev/block/platform/msm_sdcc.1/by-name/apnhlos\n")
        fileio.write(f, fileio.whitespace(line) + "mount vfat /dev/block/platform/msm_sdcc.1/by-name/apnhlos /firmware ro shortname=lower,uid=1000,gid=1000,dmask=227,fmask=337\n")

      if move_mdm_mount:
        fileio.write(f, fileio.whitespace(line) + "wait /dev/block/platform/msm_sdcc.1/by-name/mdm\n")
        fileio.write(f, fileio.whitespace(line) + "mount vfat /dev/block/platform/msm_sdcc.1/by-name/mdm /firmware-mdm ro shortname=lower,uid=1000,gid=1000,dmask=227,fmask=337\n")

    else:
      fileio.write(f, line)

    previous_line = line

  f.close()

def patch_ramdisk(directory, partition_config):
  modify_init_rc(directory)
  modify_init_qcom_rc(directory)
  modify_fstab(directory, partition_config)
  modify_init_target_rc(directory)
