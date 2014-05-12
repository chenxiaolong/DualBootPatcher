from multiboot.fileinfo import FileInfo
import multiboot.autopatcher as autopatcher
import multiboot.fileio as fileio
import re

file_info = FileInfo()

filename_regex           = r"^Slim_aroma_selectable_gapps.*\.zip$"
file_info.name           = 'SlimRoms AROMA Google Apps'
file_info.patch          = [ autopatcher.auto_patch ]
file_info.extract        = autopatcher.files_to_auto_patch
file_info.has_boot_image = False

def matches(filename):
  if re.search(filename_regex, filename):
    return True
  else:
    return False

def get_file_info():
  return file_info

def handle_bundled_mount(directory, bootimg = None, device_check = True,
                         partition_config = None, device = True):
  updater_script = 'META-INF/com/google/android/updater-script'
  lines = fileio.all_lines(updater_script, directory = directory)

  i = 0
  while i < len(lines):
    if re.search('/tmp/mount.*/system', lines[i]):
      del lines[i]
      i += autopatcher.insert_mount_system(i, lines)

    elif re.search('/tmp/mount.*/cache', lines[i]):
      del lines[i]
      i += autopatcher.insert_mount_cache(i, lines)

    elif re.search('/tmp/mount.*/data', lines[i]):
      del lines[i]
      i += autopatcher.insert_mount_data(i, lines)

    else:
      i += 1

  fileio.write_lines(updater_script, lines, directory = directory)

file_info.patch.append(handle_bundled_mount)
