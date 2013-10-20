from fileinfo import FileInfo
import os, re

file_info = FileInfo()

filename_regex           = r"^KT-SGS4-JB4.3-AOSP-.*\.zip$"
file_info.ramdisk        = os.path.join('jflte', 'AOSP', 'ktoonsez.dualboot.cpio')
file_info.patch          = os.path.join('jflte', 'Kernels', 'AOSP', 'ktoonsez.dualboot.patch')

def matches(filename):
  if re.search(filename_regex, filename):
    return True
  else:
    return False

def print_message():
  print("Detected ktoonsez kernel zip")
  print("Using patched ktoonsez ramdisk")

def get_file_info():
  return file_info
