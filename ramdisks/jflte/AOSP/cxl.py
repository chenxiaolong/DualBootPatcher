#!/usr/bin/env python3

import multiboot.fileio as fileio

import os
import re
import shutil
import sys

def modify_fstab(directory, partition_config):
  lines = fileio.all_lines('fstab.qcom', directory = directory)

  system_fourth = 'ro,barrier=1,errors=panic'
  system_fifth = 'wait'
  cache_fourth = 'noatime,nosuid,nodev,journal_async_commit'
  cache_fifth = 'wait,check'

  f = fileio.open_file('fstab.qcom', fileio.WRITE, directory = directory)
  for line in lines:
    if re.search(r"^/dev[a-zA-Z0-9/\._-]+\s+/raw-system\s+.*$", line):
      if '/raw-system' in partition_config.target_cache:
        r = re.search(r"^([^\ ]+)\s+([^\ ]+)\s+([^\ ]+)\s+([^\ ]+)\s+([^\ ]+)", line)
        line = "%s %s %s %s %s\n" % (r.groups()[0], r.groups()[1], r.groups()[2],
                                     cache_fourth, cache_fifth)

      fileio.write(f, line)

    elif re.search(r"^/dev[^\s]+\s+/raw-cache\s+.*$", line):
      if '/raw-cache' in partition_config.target_system:
        # /raw-cache needs to always be mounted rw so OpenDelta can write to
        # /cache/recovery
        args = system_fourth
        if 'ro' in args:
            args = re.sub('ro', 'rw', args)
        else:
            args = 'rw,' + args

        r = re.search(r"^([^\ ]+)\s+([^\ ]+)\s+([^\ ]+)\s+([^\ ]+)\s+([^\ ]+)", line)
        line = "%s %s %s %s %s\n" % (r.groups()[0], r.groups()[1], r.groups()[2],
                                     args, system_fifth)

      fileio.write(f, line)

    elif re.search(r"^/dev[^\s]+\s+/raw-data\s+.*$", line):
      if '/raw-data' in partition_config.target_system:
        r = re.search(r"^([^\ ]+)\s+([^\ ]+)\s+([^\ ]+)\s+([^\ ]+)\s+([^\ ]+)", line)
        line = "%s %s %s %s %s\n" % (r.groups()[0], r.groups()[1], r.groups()[2],
                                     system_fourth, system_fifth)

      fileio.write(f, line)

    else:
      fileio.write(f, line)

  f.close()

def modify_init_target_rc(directory):
  lines = fileio.all_lines('init.target.rc', directory = directory)

  f = fileio.open_file('init.target.rc', fileio.WRITE, directory = directory)
  for line in lines:
    if 'init.dualboot.mounting.sh' in line:
      fileio.write(f, re.sub('init.dualboot.mounting.sh', 'init.multiboot.mounting.sh', line))

    else:
      fileio.write(f, line)

  f.close()

def patch_ramdisk(directory, partition_config):
  modify_fstab(directory, partition_config)
  modify_init_target_rc(directory)
