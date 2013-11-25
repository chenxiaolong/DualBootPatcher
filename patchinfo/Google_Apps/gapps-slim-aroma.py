from fileinfo import FileInfo
import common as c
import re

file_info = FileInfo()

filename_regex           = r"^Slim_aroma_selectable_gapps.*\.zip$"
file_info.patch          = [ c.auto_patch ]
file_info.extract        = c.files_to_auto_patch
file_info.has_boot_image = False

def matches(filename):
  if re.search(filename_regex, filename):
    return True
  else:
    return False

def print_message():
  print("Detected Slim Bean AROMA Google Apps zip")

def get_file_info():
  return file_info

def handle_bundled_mount(directory, bootimg = None, device_check = True,
                         partition_config = None):
  lines = c.get_lines_from_file(directory, 'META-INF/com/google/android/updater-script')

  i = 0
  while i < len(lines):
    if re.search('/tmp/mount.*/system', lines[i]):
      del lines[i]
      i += c.insert_mount_system(i, lines)

    elif re.search('/tmp/mount.*/cache', lines[i]):
      del lines[i]
      i += c.insert_mount_cache(i, lines)

    elif re.search('/tmp/mount.*/data', lines[i]):
      del lines[i]
      i += c.insert_mount_data(i, lines)

    else:
      i += 1

  c.write_lines_to_file(directory, 'META-INF/com/google/android/updater-script', lines)

file_info.patch.append(handle_bundled_mount)
