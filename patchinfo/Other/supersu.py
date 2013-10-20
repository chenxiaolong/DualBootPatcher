from fileinfo import FileInfo
import re

file_info = FileInfo()

filename_regex           = r"^UPDATE-SuperSU-v[0-9\.]+\.zip$"
file_info.patch          = 'Other/supersu.dualboot.patch'
file_info.has_boot_image = False

def matches(filename):
  if re.search(filename_regex, filename):
    return True
  else:
    return False

def print_message():
  print("Detected Chainfire's SuperSU zip")

def get_file_info():
  return file_info
