#!/usr/bin/env python3

import multiboot.cmd as cmd
import multiboot.fileio as fileio
import multiboot.operatingsystem as OS

import os
import re
import shutil

# jb43, kk44
version = None

def modify_init_rc(cpiofile):
  cpioentry = cpiofile.get_file('init.rc')
  lines = fileio.bytes_to_lines(cpioentry.content)
  buf = bytes()

  previous_line = ""

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

    elif version == 'kk44' \
        and re.search(r"mount.*/system", line) \
        and re.search(r"on\s+charger", previous_line):
      buf += fileio.encode("    mount_all fstab.jgedlte\n")
      buf += fileio.encode("    exec /sbin/busybox-static sh /init.multiboot.mounting.sh\n")

    else:
      buf += fileio.encode(line)

    previous_line = line

  cpioentry.set_content(buf)

def modify_init_qcom_rc(cpiofile):
  for i in [ 'init.qcom.rc', 'init.jgedlte.rc' ]:
    cpioentry = cpiofile.get_file(i)
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
  # Ignore all contents for Google Edition
  for i in [ 'fstab.qcom', 'fstab.jgedlte' ]:
    cpioentry = cpiofile.get_file(i)
    lines = fileio.bytes_to_lines(cpioentry.content)
    buf = bytes()

    if version == 'kk44':
      system = "/dev/block/platform/msm_sdcc.1/by-name/system /raw-system ext4 ro,barrier=1 wait\n"
      cache = "/dev/block/platform/msm_sdcc.1/by-name/cache /raw-cache ext4 nosuid,nodev,barrier=1 wait,check\n"
      data = "/dev/block/platform/msm_sdcc.1/by-name/userdata /raw-data ext4 nosuid,nodev,barrier=1,noauto_da_alloc wait,check,encryptable=footer\n"

      system_fourth = 'ro,barrier=1'
      system_fifth = 'wait'
      cache_fourth = 'nosuid,nodev,barrier=1'
      cache_fifth = 'wait,check'

    elif version == 'jb43':
      system = "/dev/block/platform/msm_sdcc.1/by-name/system /raw-system ext4 ro,errors=panic wait\n"
      cache = "/dev/block/platform/msm_sdcc.1/by-name/cache /raw-cache ext4 nosuid,nodev,barrier=1 wait,check\n"
      data = "/dev/block/platform/msm_sdcc.1/by-name/userdata /raw-data ext4 nosuid,nodev,noatime,noauto_da_alloc,discard,journal_async_commit,errors=panic wait,check,encryptable=footer\n"

      system_fourth = 'ro,barrier=1,errors=panic'
      system_fifth = 'wait'
      cache_fourth = 'nosuid,nodev,barrier=1'
      cache_fifth = 'wait,check'

    # Target cache on /system partition
    target_cache_on_system = "/dev/block/platform/msm_sdcc.1/by-name/system /raw-system ext4 %s %s\n" % (cache_fourth, cache_fifth)
    # Target system on /cache partition
    target_system_on_cache = "/dev/block/platform/msm_sdcc.1/by-name/cache /raw-cache ext4 %s %s\n" % (system_fourth, system_fifth)
    # Target system on /data partition
    target_system_on_data = "/dev/block/platform/msm_sdcc.1/by-name/userdata /raw-data ext4 %s %s\n" % (system_fourth, system_fifth)

    has_cache_line = False

    for line in lines:
      if re.search(r"^/dev[a-zA-Z0-9/\._-]+\s+/system\s+.*$", line):
        if '/raw-system' in partition_config.target_cache:
          buf += fileio.encode(target_cache_on_system)
        else:
          buf += fileio.encode(system)

      elif re.search(r"^/dev[^\s]+\s+/cache\s+.*$", line):
        if '/raw-cache' in partition_config.target_system:
          buf += fileio.encode(target_system_on_cache)
        else:
          buf += fileio.encode(cache)

        has_cache_line = True

      elif re.search(r"^/dev[^\s]+\s+/data\s+.*$", line):
        if '/raw-data' in partition_config.target_system:
          buf += fileio.encode(target_system_on_data)
        else:
          buf += fileio.encode(data)

      else:
        buf += fileio.encode(line)

    if not has_cache_line:
      if '/raw-cache' in partition_config.target_system:
        buf += fileio.encode(target_system_on_cache)
      else:
        buf += fileio.encode(cache)

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

    elif re.search(r"^\s+mount_all\s+fstab.jgedlte.*$", line) and \
        re.search(r"^on\s+fs(_selinux)?.*$", previous_line):
      buf += fileio.encode(line)
      buf += fileio.encode(fileio.whitespace(line) + "exec /sbin/busybox-static sh /init.multiboot.mounting.sh\n")

    else:
      buf += fileio.encode(line)

    previous_line = line

  cpioentry.set_content(buf)

def modify_MSM8960_lpm_rc(cpiofile):
  if version == 'kk44':
    return

  cpioentry = cpiofile.get_file('MSM8960_lpm.rc')
  lines = fileio.bytes_to_lines(cpioentry.content)
  buf = bytes()

  for line in lines:
    if re.search(r"^\s+mount.*/cache.*$", line):
      buf += fileio.encode(re.sub(r"^", "#", line))

    else:
      buf += fileio.encode(line)

  cpioentry.set_content(buf)

def patch_ramdisk(cpiofile, partition_config):
  ui = OS.ui

  global version

  if cpiofile.get_file('MSM8960_lpm.rc') is not None:
    version = 'jb43'
  else:
    version = 'kk44'

  modify_init_rc(cpiofile)
  modify_init_qcom_rc(cpiofile)
  modify_fstab(cpiofile, partition_config)
  modify_init_target_rc(cpiofile)
  modify_MSM8960_lpm_rc(cpiofile)

  # Samsung's init binary is pretty screwed up
  if version == 'kk44':
    newinit = os.path.join(OS.ramdiskdir, 'init-kk44')
    cpiofile.add_file(newinit, name='init', perms=0o755)
