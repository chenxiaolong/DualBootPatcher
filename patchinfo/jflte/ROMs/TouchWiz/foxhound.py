from fileinfo import FileInfo
import common as c
import re

file_info = FileInfo()

filename_regex           = r"^FoxHound_.*\.zip$"
file_info.ramdisk        = 'jflte/TouchWiz/TouchWiz.def'
file_info.bootimg        = "snakes/Kernels/Stock/boot.img"
file_info.patch          = [ c.auto_patch,
                             'jflte/ROMs/TouchWiz/foxhound.dualboot.patch' ]
file_info.extract        = [ c.files_to_auto_patch,
                             'META-INF/com/google/android/aroma-config' ]

def matches(filename):
  if re.search(filename_regex, filename):
    return True
  else:
    return False

def print_message():
  print("Detected FoxHound ROM zip")

def get_file_info():
  return file_info
