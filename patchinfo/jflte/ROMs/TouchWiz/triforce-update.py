from multiboot.fileinfo import FileInfo
import multiboot.autopatcher as autopatcher
import multiboot.fileio as fileio
import re

file_info = FileInfo()

filename_regex           = r"^TriForceROM[0-9\.]+Update\.zip$"
file_info.name           = 'TriForceROM Update'
file_info.ramdisk        = 'jflte/TouchWiz/TouchWiz.def'
file_info.patch          = [ autopatcher.auto_patch ]
file_info.extract        = autopatcher.files_to_auto_patch

def matches(filename):
  if re.search(filename_regex, filename):
    return True
  else:
    return False

def get_file_info():
  return file_info

def fix_aroma(directory, bootimg = None, device_check = True,
              partition_config = None, device = None):
  updater_script = 'META-INF/com/google/android/updater-script'
  lines = fileio.all_lines(updater_script, directory = directory)

  i = 0
  while i < len(lines):
    if re.search('getprop.*/system/build.prop', lines[i]):
      i += autopatcher.insert_mount_system(i, lines)
      i += autopatcher.insert_mount_cache(i, lines)
      i += autopatcher.insert_mount_data(i, lines)
      lines[i] = re.sub('/system', partition_config.target_system, lines[i])

    i += 1

  fileio.write_lines(updater_script, lines, directory = directory)

file_info.patch.append(fix_aroma)
