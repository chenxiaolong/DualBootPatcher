from fileinfo import FileInfo
import os, re

file_info = FileInfo()

filename_regex           = r"^JellyBeer-.*\.zip$"
file_info.ramdisk        = os.path.join('jflte', 'AOSP', 'jellybeer.dualboot.cpio')
file_info.patch          = os.path.join('jflte', 'ROMs', 'AOSP', 'jellybeer.dualboot.patch')

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
