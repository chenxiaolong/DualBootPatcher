from fileinfo import FileInfo
import os, re

file_info = FileInfo()

filename_regex           = r"^FoxHound_.*\.zip$"
file_info.ramdisk        = os.path.join('jflte', 'TouchWiz', 'touchwiz.dualboot.cpio')
file_info.patch          = os.path.join('jflte', 'ROMs', 'TouchWiz', 'foxhound.dualboot.patch')
file_info.bootimg        = "snakes/Kernels/Stock/boot.img"

def matches(filename):
  if re.search(filename_regex, filename):
    return True
  else:
    return False

def print_message():
  print("Detected FoxHound ROM zip")
  print("Using patched TouchWiz ramdisk")

def get_file_info():
  return file_info
