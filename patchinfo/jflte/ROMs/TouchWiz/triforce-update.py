from fileinfo import FileInfo
import re

file_info = FileInfo()

filename_regex           = r"^TriForceROM[0-9\.]+Update\.zip$"
file_info.ramdisk        = 'jflte/TouchWiz/TouchWiz.def'
file_info.patch          = 'jflte/ROMs/TouchWiz/triforce-update.dualboot.patch'
file_info.bootimg        = "aroma/kernels/stock/boot.img"

def matches(filename):
  if re.search(filename_regex, filename):
    return True
  else:
    return False

def print_message():
  print("Detected TriForceROM Update zip")

def get_file_info():
  return file_info
