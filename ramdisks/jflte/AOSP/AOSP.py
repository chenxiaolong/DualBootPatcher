# Copyright (C) 2014  Xiao-Long Chen <chenxiaolong@cxl.epac.to>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import multiboot.fileio as fileio

import os
import re
import shutil
import sys

move_apnhlos_mount = False
move_mdm_mount = False

def modify_init_rc(cpiofile):
  cpioentry = cpiofile.get_file('init.rc')
  lines = fileio.bytes_to_lines(cpioentry.content)
  buf = bytes()

  for line in lines:
    if 'export ANDROID_ROOT' in line:
      buf += fileio.encode(line)
      buf += fileio.encode(fileio.whitespace(line) + "export ANDROID_CACHE /cache\n")

    elif re.search(r"mkdir /system(\s|$)", line):
      buf += fileio.encode(line)
      buf += fileio.encode(re.sub("/system", "/raw-system", line))

    elif re.search(r"mkdir /data(\s|$)", line):
      buf += fileio.encode(line)
      buf += fileio.encode(re.sub("/data", "/raw-data", line))

    elif re.search(r"mkdir /cache(\s|$)", line):
      buf += fileio.encode(line)
      buf += fileio.encode(re.sub("/cache", "/raw-cache", line))

    elif 'yaffs2' in line:
      buf += fileio.encode(re.sub(r"^", "#", line))

    else:
      buf += fileio.encode(line)

  cpioentry.set_content(buf)

def modify_init_qcom_rc(cpiofile):
  cpioentry = cpiofile.get_file('init.qcom.rc')
  lines = fileio.bytes_to_lines(cpioentry.content)
  buf = bytes()

  for line in lines:
    if 'export EMULATED_STORAGE_TARGET' in line:
      buf += fileio.encode(line)
      buf += fileio.encode(fileio.whitespace(line) + "export EXTERNAL_SD /storage/sdcard1\n")

    # Change /data/media to /raw-data/media
    elif re.search(r"/data/media(\s|$)", line):
      buf += fileio.encode(re.sub('/data/media', '/raw-data/media', line))

    else:
      buf += fileio.encode(line)

  cpioentry.set_content(buf)

def modify_fstab(cpiofile, partition_config):
  cpioentry = cpiofile.get_file('fstab.qcom')
  lines = fileio.bytes_to_lines(cpioentry.content)
  buf = bytes()

  system_fourth = 'ro,barrier=1,errors=panic'
  system_fifth = 'wait'
  cache_fourth = 'nosuid,nodev,barrier=1'
  cache_fifth = 'wait,check'

  # For Android 4.2 ROMs
  has_cache_line = False

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
      has_cache_line = True

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

    elif re.search(r"^/dev/[^\s]+apnhlos\s", line):
      global move_apnhlos_mount
      move_apnhlos_mount = True
      continue

    elif re.search(r"^/dev/[^\s]+mdm\s", line):
      global move_mdm_mount
      move_mdm_mount = True
      continue

    else:
      buf += fileio.encode(line)

  if not has_cache_line:
    if '/raw-cache' in partition_config.target_system:
      buf += fileio.encode("/dev/block/platform/msm_sdcc.1/by-name/cache /raw-cache ext4 %s %s\n" % (system_fourth, system_fifth))
    else:
      buf += fileio.encode("/dev/block/platform/msm_sdcc.1/by-name/cache /raw-cache ext4 nosuid,nodev,barrier=1 wait,check\n")

  cpioentry.set_content(buf)

def modify_init_target_rc(cpiofile):
  cpioentry = cpiofile.get_file('init.target.rc')
  lines = fileio.bytes_to_lines(cpioentry.content)
  buf = bytes()

  previous_line = ""

  for line in lines:
    if re.search(r"^\s+wait\s+/dev/.*/cache.*$", line):
      buf += fileio.encode(re.sub(r"^", "#", line))

    elif re.search(r"^\s+check_fs\s+/dev/.*/cache.*$", line):
      buf += fileio.encode(re.sub(r"^", "#", line))

    elif re.search(r"^\s+mount\s+ext4\s+/dev/.*/cache.*$", line):
      buf += fileio.encode(re.sub(r"^", "#", line))

    elif re.search(r"^\s+mount_all\s+fstab.qcom.*$", line) and \
        re.search(r"^on\s+fs\s*$", previous_line):
      buf += fileio.encode(line)
      buf += fileio.encode(fileio.whitespace(line) + "exec /sbin/busybox-static sh /init.multiboot.mounting.sh\n")

    elif re.search(r"^\s+setprop\s+ro.crypto.fuse_sdcard\s+true", line):
      buf += fileio.encode(line)

      if move_apnhlos_mount:
        buf += fileio.encode(fileio.whitespace(line) + "wait /dev/block/platform/msm_sdcc.1/by-name/apnhlos\n")
        buf += fileio.encode(fileio.whitespace(line) + "mount vfat /dev/block/platform/msm_sdcc.1/by-name/apnhlos /firmware ro shortname=lower,uid=1000,gid=1000,dmask=227,fmask=337\n")

      if move_mdm_mount:
        buf += fileio.encode(fileio.whitespace(line) + "wait /dev/block/platform/msm_sdcc.1/by-name/mdm\n")
        buf += fileio.encode(fileio.whitespace(line) + "mount vfat /dev/block/platform/msm_sdcc.1/by-name/mdm /firmware-mdm ro shortname=lower,uid=1000,gid=1000,dmask=227,fmask=337\n")

    else:
      buf += fileio.encode(line)

    previous_line = line

  cpioentry.set_content(buf)

def patch_ramdisk(cpiofile, partition_config):
  modify_init_rc(cpiofile)
  modify_init_qcom_rc(cpiofile)
  modify_fstab(cpiofile, partition_config)
  modify_init_target_rc(cpiofile)
