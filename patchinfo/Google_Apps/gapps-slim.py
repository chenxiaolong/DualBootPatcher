from fileinfo import FileInfo
import common as c
import re

file_info = FileInfo()

filename_regex           = r"^Slim_AIO_gapps.*.zip$"
file_info.patch          = c.auto_patch
file_info.extract        = c.files_to_auto_patch
file_info.has_boot_image = False

def matches(filename):
  if re.search(filename_regex, filename):
    return True
  else:
    return False

def print_message():
  print("Detected Slim Bean Google Apps zip")

def get_file_info():
  return file_info
