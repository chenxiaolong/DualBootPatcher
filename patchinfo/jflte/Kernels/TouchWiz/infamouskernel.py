from fileinfo import FileInfo
import os, re

file_info = FileInfo()

filename_regex           = r"^Infamous_S4_Kernel.v.*\.zip$"
file_info.ramdisk        = os.path.join('jflte', 'TouchWiz', 'infamouskernel.dualboot.cpio')
file_info.patch          = os.path.join('jflte', 'Kernels', 'TouchWiz', 'infamouskernel.dualboot.patch')

def matches(filename):
  if re.search(filename_regex, filename):
    return True
  else:
    return False

def print_message():
  print("Detected Infamous kernel zip")
  print("Using patched Infamous kernel ramdisk")

def get_file_info():
  return file_info
