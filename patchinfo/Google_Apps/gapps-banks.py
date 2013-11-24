from fileinfo import FileInfo
import common as c
import re

file_info = FileInfo()

filename_regex           = r"^[0-9]+-[0-9]+_GApps_Core_[0-9\.]+_signed\.zip$"
file_info.patch          = c.auto_patch
file_info.extract        = c.files_to_auto_patch
file_info.has_boot_image = False

def matches(filename):
  if re.search(filename_regex, filename):
    return True
  else:
    return False

def print_message():
  print("Detected BaNks's Core Google Apps zip")

def get_file_info():
  return file_info
