from fileinfo import FileInfo
import common as c
import re

file_info = FileInfo()

filename_regex           = r"^cm-[0-9\.]+-[0-9]+-.*.zip$"
file_info.ramdisk        = 'jflte/AOSP/AOSP.def'
file_info.patch          = c.auto_patch
file_info.extract        = c.files_to_auto_patch

def matches(filename):
  if re.search(filename_regex, filename):
    if 'noobdev' in filename:
      return False
    return True
  else:
    return False

def print_message():
  print("Detected Cyanogenmod based ROM zip")

def get_file_info(filename = ""):
  return file_info
