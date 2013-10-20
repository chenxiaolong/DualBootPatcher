from fileinfo import FileInfo
import re

file_info = FileInfo()

filename_regex           = r"^omni-[0-9\.]+-[0-9]+.*\.zip$"
file_info.ramdisk        = 'jflte/AOSP/omni.dualboot.cpio'
file_info.patch          = 'jflte/ROMs/AOSP/omni.dualboot.patch'

def matches(filename):
  if re.search(filename_regex, filename):
    return True
  else:
    return False

def print_message():
  print("Detected official Omni ROM zip")
  print("Using patched Omni ramdisk")

def get_file_info():
  return file_info
