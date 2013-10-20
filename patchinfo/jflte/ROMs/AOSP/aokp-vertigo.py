from fileinfo import FileInfo
import os, re

file_info = FileInfo()

filename_regex           = r"^aokp_ICJ.*\.zip$"
file_info.ramdisk        = os.path.join('jflte', 'AOSP', 'aokp.dualboot.cpio')
file_info.patch          = os.path.join('jflte', 'ROMs', 'AOSP', 'aokp.dualboot.patch')

def matches(filename):
  if re.search(filename_regex, filename):
    return True
  else:
    return False

def print_message():
  print("Detected Vertigo's AOKP ROM zip")
  print("Using patched AOKP ramdisk")

def get_file_info():
  return file_info
