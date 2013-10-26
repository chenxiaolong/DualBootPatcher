from fileinfo import FileInfo
import re

file_info = FileInfo()

filename_regex           = r"^JellyBeer-.*\.zip$"
file_info.ramdisk        = 'jflte/AOSP/AOSP.def'
file_info.patch          = 'jflte/ROMs/AOSP/jellybeer.dualboot.patch'
file_info.need_new_init  = True

def matches(filename):
  if re.search(filename_regex, filename):
    return True
  else:
    return False

def print_message():
  print("Detected JellyBeer ROM zip")

def get_file_info():
  return file_info
