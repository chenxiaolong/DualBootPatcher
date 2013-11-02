from fileinfo import FileInfo
import re

file_info = FileInfo()

filename_regex           = r"^omni-[0-9\.]+-[0-9]+.*\.zip$"
file_info.ramdisk        = 'jflte/AOSP/AOSP.def'

def matches(filename):
  if re.search(filename_regex, filename):
    if '4.4' in filename:
      file_info.patch    = 'jflte/ROMs/AOSP/omni-kitkat.dualboot.patch'
    else:
      file_info.patch    = 'jflte/ROMs/AOSP/omni.dualboot.patch'
    return True
  else:
    return False

def print_message():
  print("Detected official Omni ROM zip")

def get_file_info():
  return file_info
