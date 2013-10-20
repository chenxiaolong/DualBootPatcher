from fileinfo import FileInfo
import os, re

file_info = FileInfo()

filename_regex           = r"^pa_[a-z0-9]+-.*-[0-9]+.zip$"
file_info.ramdisk        = os.path.join('jflte', 'AOSP', 'paranoidandroid.dualboot.cpio')
file_info.patch          = os.path.join('jflte', 'ROMs', 'AOSP', 'paranoidandroid.dualboot.patch')

def matches(filename):
  if re.search(filename_regex, filename):
    return True
  else:
    return False

def print_message():
  print("Detected ParanoidAndroid ROM zip")
  print("Using patched ParanoidAndroid ramdisk")

def get_file_info():
  return file_info
