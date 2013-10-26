from fileinfo import FileInfo
import re

file_info = FileInfo()

filename_regex           = r"^aosp43-i9505-.*\.zip$"
file_info.ramdisk        = 'jflte/AOSP/AOSP.def'
file_info.patch          = 'jflte/ROMs/AOSP/aosp.dualboot.patch'

def matches(filename):
  if re.search(filename_regex, filename):
    return True
  else:
    return False

def print_message():
  print("Detected Broodplank's AOSP ROM zip")

def get_file_info():
  return file_info
