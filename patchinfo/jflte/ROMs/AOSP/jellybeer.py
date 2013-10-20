from fileinfo import FileInfo
import re

file_info = FileInfo()

filename_regex           = r"^JellyBeer-.*\.zip$"
file_info.ramdisk        = 'jflte/AOSP/jellybeer.dualboot.cpio'
file_info.patch          = 'jflte/ROMs/AOSP/jellybeer.dualboot.patch'

def matches(filename):
  if re.search(filename_regex, filename):
    return True
  else:
    return False

def print_message():
  print("Detected JellyBeer ROM zip")
  print("Using patched JellyBeer ramdisk")

def get_file_info():
  return file_info
