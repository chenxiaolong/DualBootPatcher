import os
import re
import sys

## File manipulation

READ = 'r'
WRITE = 'wb'

def write(f, line):
  if sys.hexversion < 0x03000000:
    f.write(line)
  else:
    f.write(line.encode("UTF-8"))

def open_file(directory, f, flags):
  return open(os.path.join(directory, f), flags)

def get_lines_from_file(directory, f):
  f = open_file(directory, f, READ)
  lines = f.readlines()
  f.close()
  return lines

def write_lines_to_file(directory, f, lines):
  f = open_file(directory, f, WRITE)

  for i in lines:
    write(f, i)

  f.close()

def whitespace(line):
  m = re.search(r"^(\s+).*$", line)
  if m and m.group(1):
    return m.group(1)
  else:
    return ""

## Other stuff

def insert_line(index, line, lines):
  #lines.insert(index, (line + '\n').encode("UTF-8"))
  lines.insert(index, line + '\n')
  return 1

def insert_dual_boot_sh(lines):
  i = 0
  i += insert_line(i, 'package_extract_file("dualboot.sh", "/tmp/dualboot.sh");', lines)
  i += insert_line(i, 'set_perm(0, 0, 0777, "/tmp/dualboot.sh");', lines)
  # TODO: Word this better for sure
  i += insert_line(i, 'ui_print("NOT INSTALLING AS PRIMARY");', lines)
  i += insert_unmount_everything(i, lines)
  return i

def insert_write_kernel(bootimg, lines):
  # Look for last line containing the boot image string and insert after that
  i = len(lines) - 1
  while i > 0:
    if bootimg in lines[i]:
      # Statements can be on multiple lines, so insert after a semicolon is found
      while i < len(lines):
        if ';' in lines[i]:
          insert_line(i + 1, 'run_program("/tmp/dualboot.sh", "set-multi-kernel");', lines)
          break

        else:
          i += 1

      break
    i -= 1

def insert_mount_system(index, lines):
  return insert_line(index, 'run_program("/tmp/dualboot.sh", "mount-system");', lines)

def insert_mount_cache(index, lines):
  return insert_line(index, 'run_program("/tmp/dualboot.sh", "mount-cache");', lines)

def insert_mount_data(index, lines):
  return insert_line(index, 'run_program("/tmp/dualboot.sh", "mount-data");', lines)

def insert_unmount_system(index, lines):
  return insert_line(index, 'run_program("/tmp/dualboot.sh", "unmount-system");', lines)

def insert_unmount_cache(index, lines):
  return insert_line(index, 'run_program("/tmp/dualboot.sh", "unmount-cache");', lines)

def insert_unmount_everything(index, lines):
  return insert_line(index, 'run_program("/tmp/dualboot.sh", "unmount-everything");', lines)

def insert_unmount_data(index, lines):
  return insert_line(index, 'run_program("/tmp/dualboot.sh", "unmount-data");', lines)

def insert_format_system(index, lines):
  return insert_line(index, 'run_program("/tmp/dualboot.sh", "format-system");', lines)

def insert_format_cache(index, lines):
  return insert_line(index, 'run_program("/tmp/dualboot.sh", "format-cache");', lines)

def insert_format_data(index, lines):
  return insert_line(index, 'run_program("/tmp/dualboot.sh", "format-data");', lines)

def replace_mount_lines(lines):
  i = 0
  while i < len(lines):
    if re.search(r"^\s*mount\s*\(.*$", lines[i]) or \
       re.search(r"^\s*run_program\s*\(\s*\"[^\"]*busybox\"\s*,\s*\"mount\".*$", lines[i]):
      # Mount /system as dual boot
      if 'system' in lines[i] or 'mmcblk0p16' in lines[i]:
        del lines[i]
        i += insert_mount_system(i, lines)

      # Mount /cache as dual boot
      elif 'cache' in lines[i] or 'mmcblk0p18' in lines[i]:
        del lines[i]
        i += insert_mount_cache(i, lines)

      # Mount /data as dual boot
      elif '"/data' in lines[i] or 'userdata' in lines[i] or 'mmcblk0p29' in lines[i]:
        del lines[i]
        i += insert_mount_data(i, lines)

      else:
        i += 1

    else:
      i += 1

def replace_unmount_lines(lines):
  i = 0
  while i < len(lines):
    if re.search(r"^\s*unmount\s*\(.*$", lines[i]) or \
       re.search(r"^\s*run_program\s*\(\s*\"[^\"]*busybox\"\s*,\s*\"umount\".*$", lines[i]):
      # Mount /system as dual boot
      if 'system' in lines[i] or 'mmcblk0p16' in lines[i]:
        del lines[i]
        i += insert_unmount_system(i, lines)

      # Mount /cache as dual boot
      elif 'cache' in lines[i] or 'mmcblk0p18' in lines[i]:
        del lines[i]
        i += insert_unmount_cache(i, lines)

      # Mount /data as dual boot
      elif '"/data' in lines[i] or 'userdata' in lines[i] or 'mmcblk0p29' in lines[i]:
        del lines[i]
        i += insert_unmount_data(i, lines)

      else:
        i += 1

    else:
      i += 1

def replace_format_lines(lines):
  i = 0
  while i < len(lines):
    if re.search(r"^\s*format\s*\(.*$", lines[i]):
      # Mount /system as dual boot
      if 'system' in lines[i] or 'mmcblk0p16' in lines[i]:
        del lines[i]
        i += insert_format_system(i, lines)

      # Mount /cache as dual boot
      elif 'cache' in lines[i] or 'mmcblk0p18' in lines[i]:
        del lines[i]
        i += insert_format_cache(i, lines)

      # Mount /data as dual boot
      elif 'userdata' in lines[i] or 'mmcblk0p29' in lines[i]:
        del lines[i]
        i += insert_format_data(i, lines)

      else:
        i += 1

    elif re.search(r'delete_recursive\s*\([^\)]*"/system"', lines[i]):
      del lines[i]
      i += insert_format_system(i, lines)

    elif re.search(r'delete_recursive\s*\([^\)]*"/cache"', lines[i]):
      del lines[i]
      i += insert_format_cache(i, lines)

    else:
      i += 1

def remove_device_checks(lines):
  i = 0
  while i < len(lines):
    if re.search(r"^\s*assert\s*\(.*getprop\s*\(.*(ro.product.device|ro.build.product)", lines[i]):
      lines[i] = re.sub(r"^(\s*assert\s*\()", r"""\1"true" == "true" || """, lines[i])

    i += 1

def attempt_auto_patch(lines, bootimg = None, device_check = True,
                       partition_config = None):
  insert_dual_boot_sh(lines)
  replace_mount_lines(lines)
  replace_unmount_lines(lines)
  replace_format_lines(lines)
  if bootimg:
    insert_write_kernel(bootimg, lines)
  # Too many ROMs don't unmount partitions after installation
  insert_unmount_everything(len(lines), lines)
  # Remove device checks
  if not device_check:
    remove_device_checks(lines)

# Functions to assign to FileInfo.patch
def auto_patch(directory, bootimg = None, device_check = True,
               partition_config = None):
  lines = get_lines_from_file(directory, 'META-INF/com/google/android/updater-script')
  attempt_auto_patch(lines, bootimg = bootimg, device_check = device_check,
                     partition_config = partition_config)
  write_lines_to_file(directory, 'META-INF/com/google/android/updater-script', lines)

# Functions to assign to FileInfo.extract
def files_to_auto_patch():
  return [ 'META-INF/com/google/android/updater-script' ]
