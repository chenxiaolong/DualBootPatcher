from fileinfo import FileInfo
import re

file_info = FileInfo()

filename_regex           = r"^GE-.*\.zip$"
file_info.ramdisk        = 'jflte/TouchWiz/TouchWiz.def'
file_info.patch          = "jflte/ROMs/TouchWiz/goldeneye.dualboot.patch"
file_info.bootimg        = "kernel/boot.img"

def matches(filename):
  if re.search(filename_regex, filename):
    if 'GE-SGH-M919' in filename:
      return True
    else:
      return False
  else:
    return False

def print_message():
  print("Detected GoldenEye ROM zip")

def get_file_info():
  return file_info
