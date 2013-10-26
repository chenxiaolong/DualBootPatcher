from fileinfo import FileInfo
import re

file_info = FileInfo()

filename_regex           = r"^FoxHound_.*\.zip$"
file_info.ramdisk        = 'jflte/TouchWiz/TouchWiz.def'
file_info.bootimg        = "snakes/Kernels/Stock/boot.img"

def matches(filename):
  if re.search(filename_regex, filename):
    if re.search("_3.[0-9]+", filename):
      file_info.patch    = 'jflte/ROMs/TouchWiz/foxhound-3.0.dualboot.patch'
    else:
      file_info.patch    = 'jflte/ROMs/TouchWiz/foxhound.dualboot.patch'
    return True
  else:
    return False

def print_message():
  print("Detected FoxHound ROM zip")

def get_file_info():
  return file_info
