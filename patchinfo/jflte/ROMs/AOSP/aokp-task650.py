from fileinfo import FileInfo
import os, re

file_info = FileInfo()

filename_regex           = r"^aokp_[0-9\.]+_[a-z0-9]+_task650_[0-9\.]+\.zip$"
file_info.ramdisk        = os.path.join('jflte', 'AOSP', 'aokp-task650.dualboot.cpio')
file_info.patch          = os.path.join('jflte', 'ROMs', 'AOSP', 'aokp.dualboot.patch')

def matches(filename):
  if re.search(filename_regex, filename):
    return True
  else:
    return False

def print_message():
  print("Detected Task650's AOKP ROM zip")
  print("Using patched Task650's AOKP ramdisk")

def get_file_info():
  return file_info
