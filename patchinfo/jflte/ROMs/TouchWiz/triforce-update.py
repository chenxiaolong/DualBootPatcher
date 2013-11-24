from fileinfo import FileInfo
import common as c
import re

file_info = FileInfo()

filename_regex           = r"^TriForceROM[0-9\.]+Update\.zip$"
file_info.ramdisk        = 'jflte/TouchWiz/TouchWiz.def'
file_info.patch          = [ c.auto_patch ]
file_info.extract        = c.files_to_auto_patch

def matches(filename):
  if re.search(filename_regex, filename):
    return True
  else:
    return False

def print_message():
  print("Detected TriForceROM Update zip")

def get_file_info():
  return file_info

def fix_aroma(directory, bootimg = None, device_check = True,
              partition_config = None):
  lines = c.get_lines_from_file(directory, 'META-INF/com/google/android/updater-script')

  i = 0
  while i < len(lines):
    if re.search('getprop.*/system/build.prop', lines[i]):
      i += c.insert_mount_system(i, lines)
      i += c.insert_mount_cache(i, lines)
      i += c.insert_mount_data(i, lines)
      lines[i] = re.sub('/system', partition_config.target_system, lines[i])

    i += 1

  c.write_lines_to_file(directory, 'META-INF/com/google/android/updater-script', lines)

file_info.patch.append(fix_aroma)
