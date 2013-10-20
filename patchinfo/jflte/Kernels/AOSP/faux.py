from fileinfo import FileInfo
import os, re

file_info = FileInfo()

filename_regex           = r"^jflte[a-z]+-aosp-faux123-.*\.zip$"
file_info.ramdisk        = os.path.join('jflte', 'AOSP', 'cyanogenmod.dualboot.cpio')
file_info.patch          = os.path.join('jflte', 'Kernels', 'AOSP', 'faux.dualboot.patch')

def matches(filename):
  if re.search(filename_regex, filename):
    return True
  else:
    return False

def print_message():
  print("Detected faux kernel zip")
  print("Using patched Cyanogenmod ramdisk (compatible with faux)")

def get_file_info():
  return file_info
