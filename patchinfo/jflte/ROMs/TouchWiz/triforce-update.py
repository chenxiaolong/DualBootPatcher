from fileinfo import FileInfo
import os, re

file_info = FileInfo()

filename_regex           = r"^TriForceROM[0-9\.]+Update\.zip$"
file_info.ramdisk        = os.path.join('jflte', 'TouchWiz', 'touchwiz.dualboot.cpio')
file_info.patch          = os.path.join('jflte', 'ROMs', 'TouchWiz', 'triforce-update.dualboot.patch')
file_info.bootimg        = "aroma/kernels/stock/boot.img"

def matches(filename):
  if re.search(filename_regex, filename):
    return True
  else:
    return False

def print_message():
  print("Detected TriForceROM Update zip")
  print("Using patched TouchWiz ramdisk")

def get_file_info():
  return file_info
