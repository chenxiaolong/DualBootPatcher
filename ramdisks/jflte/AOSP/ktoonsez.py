#!/usr/bin/env python3

import multiboot.fileio as fileio
import multiboot.operatingsystem as OS

import aosp
import imp
import os
import re
import shutil
import sys

aosp_ramdisk = os.path.join(OS.ramdiskdir, 'jflte', 'AOSP', 'AOSP.py')
aosp = imp.load_source(os.path.basename(aosp_ramdisk)[:-3], aosp_ramdisk)

def modify_fstab_build(cpiofile, partition_config):
  cpioentry = cpiofile.get_file('fstab.build')

  if cpioentry is None:
      return

  lines = fileio.bytes_to_lines(cpioentry.content)
  buf = bytes()

  ext4_system_fourth = 'ro,barrier=1,errors=panic'
  ext4_cache_fourth = 'nosuid,nodev,noatime,noauto_da_alloc,journal_async_commit,errors=panic'

  f2fs_system_fourth = 'ro,noatime,flush_merge,nosuid,nodev,discard,nodiratime,inline_xattr,errors=recover'
  f2fs_cache_fourth = 'noatime,flush_merge,nosuid,nodev,nodiratime,inline_xattr,errors=recover'

  system_fifth = 'wait'
  cache_fifth = 'wait,check'

  for line in lines:
    if re.search(r"^#SYS_IS(EXT4|F2FS)/dev[a-zA-Z0-9/\._-]+\s+/system\s+.*$", line):
      temp = re.sub("\s/system\s", " /raw-system ", line)

      if '/raw-system' in partition_config.target_cache:
        r = re.search(r"^([^\ ]+)\s+([^\ ]+)\s+([^\ ]+)\s+([^\ ]+)\s+([^\ ]+)", temp)
        if temp.startswith('#SYS_ISEXT4'):
          temp = "%s %s %s %s %s\n" % (r.groups()[0], r.groups()[1], r.groups()[2],
                                       ext4_cache_fourth, cache_fifth)
        elif temp.startswith('#SYS_ISF2FS'):
          temp = "%s %s %s %s %s\n" % (r.groups()[0], r.groups()[1], r.groups()[2],
                                       f2fs_cache_fourth, cache_fifth)

      buf += fileio.encode(temp)

    elif re.search(r"^#CACHE_IS(EXT4|F2FS)/dev[^\s]+\s+/cache\s+.*$", line):
      temp = re.sub("\s/cache\s", " /raw-cache ", line)

      if '/raw-cache' in partition_config.target_system:
        r = re.search(r"^([^\ ]+)\s+([^\ ]+)\s+([^\ ]+)\s+([^\ ]+)\s+([^\ ]+)", temp)
        if temp.startswith('#CACHE_ISEXT4'):
          temp = "%s %s %s %s %s\n" % (r.groups()[0], r.groups()[1], r.groups()[2],
                                       ext4_system_fourth, system_fifth)
        elif temp.startswith('#CACHE_ISF2FS'):
          temp = "%s %s %s %s %s\n" % (r.groups()[0], r.groups()[1], r.groups()[2],
                                       f2fs_system_fourth, system_fifth)

      buf += fileio.encode(temp)

    elif re.search(r"^#DATA_IS(EXT4|F2FS)/dev[^\s]+\s+/data\s+.*$", line):
      temp = re.sub("\s/data\s", " /raw-data ", line)

      if '/raw-data' in partition_config.target_system:
        r = re.search(r"^([^\ ]+)\s+([^\ ]+)\s+([^\ ]+)\s+([^\ ]+)\s+([^\ ]+)", temp)
        if temp.startswith('#DATA_ISEXT4'):
          temp = "%s %s %s %s %s\n" % (r.groups()[0], r.groups()[1], r.groups()[2],
                                       ext4_system_fourth, system_fifth)
        elif temp.startswith('#DATA_ISF2FS'):
          temp = "%s %s %s %s %s\n" % (r.groups()[0], r.groups()[1], r.groups()[2],
                                       f2fs_system_fourth, system_fifth)

      buf += fileio.encode(temp)

    elif re.search(r"^/dev/[^\s]+apnhlos\s", line):
      global move_apnhlos_mount
      aosp.move_apnhlos_mount = True
      continue

    elif re.search(r"^/dev/[^\s]+mdm\s", line):
      global move_mdm_mount
      aosp.move_mdm_mount = True
      continue

    else:
      buf += fileio.encode(line)

  cpioentry.set_content(buf)

def patch_ramdisk(cpiofile, partition_config):
  aosp.modify_init_rc(cpiofile)
  aosp.modify_init_qcom_rc(cpiofile)
  aosp.modify_fstab(cpiofile, partition_config)
  modify_fstab_build(cpiofile, partition_config)
  aosp.modify_init_target_rc(cpiofile)
