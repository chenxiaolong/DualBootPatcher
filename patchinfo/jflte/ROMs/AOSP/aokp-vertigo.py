from fileinfo import FileInfo
import re

file_info = FileInfo()

filename_regex           = r"^aokp_ICJ.*\.zip$"
file_info.ramdisk        = 'jflte/AOSP/aokp.dualboot.cpio'
file_info.patch          = 'jflte/ROMs/AOSP/aokp.dualboot.patch'

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
