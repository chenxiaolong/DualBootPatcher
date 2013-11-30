#!/usr/bin/env python3

import common as c
import os
import re
import shutil
import sys

def modify_fstab(directory, partition_config):
  lines = c.get_lines_from_file(directory, 'fstab.qcom')

  system_fourth = 'ro,barrier=1,errors=panic'
  system_fifth = 'wait'
  cache_fourth = 'nosuid,nodev,barrier=1'
  cache_fifth = 'wait,check'

  f = c.open_file(directory, 'fstab.qcom', c.WRITE)
  for line in lines:
    if re.search(r"^/dev[a-zA-Z0-9/\._-]+\s+/raw-system\s+.*$", line):
      if '/raw-system' in partition_config.target_cache:
        r = re.search(r"^([^\ ]+)\s+([^\ ]+)\s+([^\ ]+)\s+([^\ ]+)\s+([^\ ]+)", line)
        line = "%s %s %s %s %s\n" % (r.groups()[0], r.groups()[1], r.groups()[2],
                                     cache_fourth, cache_fifth)

      c.write(f, line)

    elif re.search(r"^/dev[^\s]+\s+/raw-cache\s+.*$", line):
      if '/raw-cache' in partition_config.target_system:
        r = re.search(r"^([^\ ]+)\s+([^\ ]+)\s+([^\ ]+)\s+([^\ ]+)\s+([^\ ]+)", line)
        line = "%s %s %s %s %s\n" % (r.groups()[0], r.groups()[1], r.groups()[2],
                                     system_fourth, system_fifth)

      c.write(f, line)

    elif re.search(r"^/dev[^\s]+\s+/raw-data\s+.*$", line):
      if '/raw-data' in partition_config.target_system:
        r = re.search(r"^([^\ ]+)\s+([^\ ]+)\s+([^\ ]+)\s+([^\ ]+)\s+([^\ ]+)", line)
        line = "%s %s %s %s %s\n" % (r.groups()[0], r.groups()[1], r.groups()[2],
                                     system_fourth, system_fifth)

      c.write(f, line)

    else:
      c.write(f, line)

  f.close()

def modify_init_target_rc(directory):
  lines = c.get_lines_from_file(directory, 'init.target.rc')

  f = c.open_file(directory, 'init.target.rc', c.WRITE)
  for line in lines:
    if 'init.dualboot.mounting.sh' in line:
      c.write(f, re.sub('init.dualboot.mounting.sh', 'init.multiboot.mounting.sh', line))

    else:
      c.write(f, line)

  f.close()

def patch_ramdisk(directory, partition_config):
  modify_fstab(directory, partition_config)
  modify_init_target_rc(directory)
