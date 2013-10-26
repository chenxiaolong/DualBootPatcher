from fileinfo import FileInfo
import re

file_info = FileInfo()

filename_regex           = r"^aicp_.*\.zip$"
file_info.ramdisk        = 'jflte/AOSP/AOSP.def'
file_info.patch          = 'jflte/ROMs/AOSP/aicp.dualboot.patch'

def matches(filename):
  if re.search(filename_regex, filename):
    return True
  else:
    return False

def print_message():
  print("Detected AICP ROM zip")

def get_file_info():
  return file_info
