from fileinfo import FileInfo
import common as c
import re

file_info = FileInfo()

filename_regex           = r"^negalite-.*\.zip"
file_info.ramdisk        = 'jflte/TouchWiz/TouchWiz.def'
file_info.patch          = [ c.auto_patch,
                             'jflte/ROMs/TouchWiz/negalite.dualboot.patch' ]
file_info.extract        = [ c.files_to_auto_patch,
                             'META-INF/com/google/android/aroma-config' ]
file_info.bootimg        = "kernel/stock_kernel/kernel/boot.img"

def matches(filename):
  if re.search(filename_regex, filename):
    return True
  else:
    return False

def print_message():
  print("Detected Negalite ROM zip")

def get_file_info():
  return file_info

def dont_wipe_data(directory, bootimg = None, device_check = True,
                   partition_config = None):
  lines = c.get_lines_from_file(directory, 'META-INF/com/google/android/updater-script')

  i = 0
  while i < len(lines):
    if re.search('run_program.*/tmp/wipedata.sh', lines[i]):
      del lines[i]
      c.insert_format_data(i, lines)
      break

    i += 1

  c.write_lines_to_file(directory, 'META-INF/com/google/android/updater-script', lines)

file_info.patch.append(dont_wipe_data)
