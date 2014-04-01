#!/usr/bin/env python3

import multiboot.fileio as fileio

import os
import re
import shutil
import sys

def modify_init_rc(cpiofile):
  cpioentry = cpiofile.get_file('init.rc')
  lines = fileio.bytes_to_lines(cpioentry.content)
  buf = bytes()

  for line in lines:
    if re.search(r"mkdir /system(\s|$)", line):
      buf += fileio.encode(line)
      buf += fileio.encode(re.sub("/system", "/raw-system", line))

    elif re.search(r"mkdir /data(\s|$)", line):
      buf += fileio.encode(line)
      buf += fileio.encode(re.sub("/data", "/raw-data", line))

    elif re.search(r"mkdir /cache(\s|$)", line):
      buf += fileio.encode(line)
      buf += fileio.encode(re.sub("/cache", "/raw-cache", line))

    else:
      buf += fileio.encode(line)

  cpioentry.set_content(buf)

def modify_init_qcom_rc(cpiofile):
  cpioentry = cpiofile.get_file('init.qcom.rc')
  lines = fileio.bytes_to_lines(cpioentry.content)
  buf = bytes()

  for line in lines:
    # Change /data/media to /raw-data/media
    if re.search(r"/data/media(\s|$)", line):
      buf += fileio.encode(re.sub('/data/media', '/raw-data/media', line))

    else:
      buf += fileio.encode(line)

  cpioentry.set_content(buf)

def modify_fstab(cpiofile, partition_config):
  for fstab in [ 'fstab.qcom', 'gpe-fstab.qcom' ]:
    cpioentry = cpiofile.get_file(fstab)
    lines = fileio.bytes_to_lines(cpioentry.content)
    buf = bytes()

    system_fourth = 'ro,barrier=1'
    system_fifth = 'wait'
    cache_fourth = 'rw,nosuid,nodev,noatime,nodiratime,data=writeback,noauto_da_alloc,barrier=1'
    cache_fifth = 'wait,check'

    for line in lines:
      if re.search(r"^/dev[a-zA-Z0-9/\._-]+\s+/system\s+.*$", line):
        temp = re.sub("\s/system\s", " /raw-system ", line)

        if '/raw-system' in partition_config.target_cache:
          r = re.search(r"^([^\ ]+)\s+([^\ ]+)\s+([^\ ]+)\s+([^\ ]+)\s+([^\ ]+)", temp)
          temp = "%s %s %s %s %s\n" % (r.groups()[0], r.groups()[1], r.groups()[2],
                                       cache_fourth, cache_fifth)

        buf += fileio.encode(temp)

      elif re.search(r"^/dev[^\s]+\s+/cache\s+.*$", line):
        temp = re.sub("\s/cache\s", " /raw-cache ", line)

        if '/raw-cache' in partition_config.target_system:
          r = re.search(r"^([^\ ]+)\s+([^\ ]+)\s+([^\ ]+)\s+([^\ ]+)\s+([^\ ]+)", temp)
          temp = "%s %s %s %s %s\n" % (r.groups()[0], r.groups()[1], r.groups()[2],
                                       system_fourth, system_fifth)

        buf += fileio.encode(temp)

      elif re.search(r"^/dev[^\s]+\s+/data\s+.*$", line):
        temp = re.sub("\s/data\s", " /raw-data ", line)

        if '/raw-data' in partition_config.target_system:
          r = re.search(r"^([^\ ]+)\s+([^\ ]+)\s+([^\ ]+)\s+([^\ ]+)\s+([^\ ]+)", temp)
          temp = "%s %s %s %s %s\n" % (r.groups()[0], r.groups()[1], r.groups()[2],
                                       system_fourth, system_fifth)

        buf += fileio.encode(temp)

      else:
        buf += fileio.encode(line)

    cpioentry.set_content(buf)

def modify_init_target_rc(cpiofile):
  cpioentry = cpiofile.get_file('init.target.rc')
  lines = fileio.bytes_to_lines(cpioentry.content)
  buf = bytes()

  previous_line = ""

  for line in lines:
    if re.search(r"^\s+mount_all\s+./fstab.qcom.*$", line) and \
        re.search(r"^on\s+fs\s*$", previous_line):
      buf += fileio.encode(line)
      buf += fileio.encode(fileio.whitespace(line) + "exec /sbin/busybox-static sh /init.multiboot.mounting.sh\n")

    # Change /data/media to /raw-data/media
    elif re.search(r"/data/media(\s|$)", line):
      buf += fileio.encode(re.sub('/data/media', '/raw-data/media', line))

    else:
      buf += fileio.encode(line)

    previous_line = line

  cpioentry.set_content(buf)

def patch_ramdisk(cpiofile, partition_config):
  modify_init_rc(cpiofile)
  modify_init_qcom_rc(cpiofile)
  modify_fstab(cpiofile, partition_config)
  modify_init_target_rc(cpiofile)
