#!/usr/bin/env python3

import multiboot.fileio as fileio

import os
import re
import shutil
import sys

def modify_fstab(cpiofile, partition_config):
  cpioentry = cpiofile.get_file('fstab.qcom')
  lines = fileio.bytes_to_lines(cpioentry.content)
  buf = bytes()

  system_fourth = 'ro,barrier=1,errors=panic'
  system_fifth = 'wait'
  cache_fourth = 'noatime,nosuid,nodev,journal_async_commit'
  cache_fifth = 'wait,check'

  for line in lines:
    if re.search(r"^/dev[a-zA-Z0-9/\._-]+\s+/raw-system\s+.*$", line):
      if '/raw-system' in partition_config.target_cache:
        r = re.search(r"^([^\ ]+)\s+([^\ ]+)\s+([^\ ]+)\s+([^\ ]+)\s+([^\ ]+)", line)
        line = "%s %s %s %s %s\n" % (r.groups()[0], r.groups()[1], r.groups()[2],
                                     cache_fourth, cache_fifth)

      buf += fileio.encode(line)

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

      buf += fileio.encode(line)

    elif re.search(r"^/dev[^\s]+\s+/raw-data\s+.*$", line):
      if '/raw-data' in partition_config.target_system:
        r = re.search(r"^([^\ ]+)\s+([^\ ]+)\s+([^\ ]+)\s+([^\ ]+)\s+([^\ ]+)", line)
        line = "%s %s %s %s %s\n" % (r.groups()[0], r.groups()[1], r.groups()[2],
                                     system_fourth, system_fifth)

      buf += fileio.encode(line)

    else:
      buf += fileio.encode(line)

  cpioentry.set_content(buf)

def modify_init_target_rc(cpiofile):
  cpioentry = cpiofile.get_file('init.target.rc')
  lines = fileio.bytes_to_lines(cpioentry.content)
  buf = bytes()

  for line in lines:
    if 'init.dualboot.mounting.sh' in line:
      buf += fileio.encode(re.sub('init.dualboot.mounting.sh', 'init.multiboot.mounting.sh', line))

    else:
      buf += fileio.encode(line)

  cpioentry.set_content(buf)

def patch_ramdisk(cpiofile, partition_config):
  modify_fstab(cpiofile, partition_config)
  modify_init_target_rc(cpiofile)
