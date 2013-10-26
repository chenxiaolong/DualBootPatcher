from fileinfo import FileInfo
import re

file_info = FileInfo()

filename_regex           = r"^probam.*\.zip$"
file_info.ramdisk        = 'jflte/AOSP/AOSP.def'
file_info.patch          = 'jflte/ROMs/AOSP/cyanogenmod.dualboot.patch'

def matches(filename):
  if re.search(filename_regex, filename):
    return True
  else:
    return False

def print_message():
  print("Detected ProBAM ROM zip")

def get_file_info():
  return file_info
