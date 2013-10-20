from fileinfo import FileInfo
import re

file_info = FileInfo()

filename_regex           = r"^TriForceROM[0-9\.]+\.zip$"
file_info.ramdisk        = 'jflte/TouchWiz/touchwiz.dualboot.cpio'
file_info.patch          = 'jflte/ROMs/TouchWiz/triforce.dualboot.patch'
file_info.bootimg        = "aroma/kernels/stock/boot.img"

def matches(filename):
  if re.search(filename_regex, filename):
    return True
  else:
    return False

def print_message():
  print("Detected TriForceROM zip")
  print("Using patched TouchWiz ramdisk")

def get_file_info():
  return file_info
