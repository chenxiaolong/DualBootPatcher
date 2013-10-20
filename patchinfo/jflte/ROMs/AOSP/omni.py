from fileinfo import FileInfo
import os, re

file_info = FileInfo()

filename_regex           = r"^omni-[0-9\.]+-[0-9]+.*\.zip$"
file_info.ramdisk        = os.path.join('jflte', 'AOSP', 'omni.dualboot.cpio')
file_info.patch          = os.path.join('jflte', 'ROMs', 'AOSP', 'omni.dualboot.patch')

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
