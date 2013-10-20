from fileinfo import FileInfo
import os, re

file_info = FileInfo()

filename_regex           = r"^v[0-9\.]+-Google-edition-ausdim-Kernel-.*\.zip$"
file_info.ramdisk        = os.path.join('jflte', 'TouchWiz', 'ausdim.dualboot.cpio')
file_info.patch          = os.path.join('jflte', 'Kernels', 'TouchWiz', 'ausdim.dualboot.patch')

def matches(filename):
  if re.search(filename_regex, filename):
    return True
  else:
    return False

def print_message():
  print("Detected Ausdim kernel zip")
  print("Using patched Ausdim kernel ramdisk")
  print("NOTE: The ramdisk is based on Ausdim v18.1. If a newer version has ramdisk changes, let me know")

def get_file_info():
  return file_info
