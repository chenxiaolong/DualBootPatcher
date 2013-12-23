#!/usr/bin/env python3

import multiboot.cmd as cmd
import multiboot.exit as exit
import multiboot.fileio as fileio
import multiboot.operatingsystem as OS

import os
import re
import shutil

# jb43, kk44
version = None

def modify_init_rc(directory):
  lines = fileio.all_lines('init.rc', directory = directory)

  previous_line = ""

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

    elif version == 'kk44' \
        and re.search(r"mount.*/system", line) \
        and re.search(r"on\s+charger", previous_line):
      fileio.write(f, "    mount_all fstab.jgedlte\n")
      fileio.write(f, "    exec /sbin/busybox-static sh /init.multiboot.mounting.sh\n")

    else:
      fileio.write(f, line)

    previous_line = line

  f.close()

def modify_init_qcom_rc(directory):
  for i in [ 'init.qcom.rc', 'init.jgedlte.rc' ]:
    lines = fileio.all_lines(i, directory = directory)

    f = fileio.open_file(i, fileio.WRITE, directory = directory)
    for line in lines:
      # Change /data/media to /raw-data/media
      if re.search(r"/data/media(\s|$)", line):
        fileio.write(f, re.sub('/data/media', '/raw-data/media', line))

      else:
        fileio.write(f, line)

    f.close()

def modify_fstab(directory, partition_config):
  # Ignore all contents for Google Edition
  for i in [ 'fstab.qcom', 'fstab.jgedlte' ]:
    lines = fileio.all_lines(i, directory = directory)

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

    f = fileio.open_file(i, fileio.WRITE, directory = directory)
    for line in lines:
      if re.search(r"^/dev[a-zA-Z0-9/\._-]+\s+/system\s+.*$", line):
        if '/raw-system' in partition_config.target_cache:
          fileio.write(f, target_cache_on_system)
        else:
          fileio.write(f, system)

      elif re.search(r"^/dev[^\s]+\s+/cache\s+.*$", line):
        if '/raw-cache' in partition_config.target_system:
          fileio.write(f, target_system_on_cache)
        else:
          fileio.write(f, cache)

        has_cache_line = True

      elif re.search(r"^/dev[^\s]+\s+/data\s+.*$", line):
        if '/raw-data' in partition_config.target_system:
          fileio.write(f, target_system_on_data)
        else:
          fileio.write(f, data)

      else:
        fileio.write(f, line)

    if not has_cache_line:
      if '/raw-cache' in partition_config.target_system:
        fileio.write(f, target_system_on_cache)
      else:
        fileio.write(f, cache)

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

    elif re.search(r"^\s+mount_all\s+fstab.jgedlte.*$", line) and \
        re.search(r"^on\s+fs(_selinux)?.*$", previous_line):
      fileio.write(f, line)
      fileio.write(f, fileio.whitespace(line) + "exec /sbin/busybox-static sh /init.multiboot.mounting.sh\n")

    else:
      fileio.write(f, line)

    previous_line = line

  f.close()

def modify_MSM8960_lpm_rc(directory):
  if version == 'kk44':
    return

  lines = fileio.all_lines('MSM8960_lpm.rc', directory = directory)

  f = fileio.open_file('MSM8960_lpm.rc', fileio.WRITE, directory = directory)
  for line in lines:
    if re.search(r"^\s+mount.*/cache.*$", line):
      fileio.write(f, re.sub(r"^", "#", line))

    else:
      fileio.write(f, line)

  f.close()

def patch_ramdisk(directory, partition_config):
  ui = OS.ui

  global version

  if os.path.exists(os.path.join(directory, 'MSM8960_lpm.rc')):
    version = 'jb43'
  else:
    version = 'kk44'

  modify_init_rc(directory)
  modify_init_qcom_rc(directory)
  modify_fstab(directory, partition_config)
  modify_init_target_rc(directory)
  modify_MSM8960_lpm_rc(directory)

  # Samsung's init binary is pretty screwed up
  if version == 'kk44':
    init = os.path.join(directory, 'init')

    os.remove(init)
    shutil.copyfile(os.path.join(OS.ramdiskdir, 'init-kk44'), init)

    # chmod 755
    if OS.is_windows():
      chmod = os.path.join(OS.binariesdir, "chmod.exe")
      exit_status, output, error = cmd.run_command(
        [ chmod, '0755', init ]
      )

      if exit_status != 0:
        ui.command_error(output = output, error = error)
        ui.failed("Failed to chmod init (WINDOWS)")
        exit.exit(1)

    else:
      import stat
      os.chmod(init, stat.S_IRUSR | stat.S_IWUSR | stat.S_IXUSR |
                     stat.S_IRGRP |                stat.S_IXGRP |
                     stat.S_IROTH |                stat.S_IXOTH)

  # SELinux hates having a directory on /cache bind-mounted to /system
#  if '/raw-cache' in partition_config.target_system:
#    path = os.path.join(directory, 'sepolicy')
#    if os.path.exists(path):
#      os.remove(path)
