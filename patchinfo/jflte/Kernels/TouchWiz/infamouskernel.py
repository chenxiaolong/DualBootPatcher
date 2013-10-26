from fileinfo import FileInfo
import re

file_info = FileInfo()

filename_regex           = r"^Infamous_S4_Kernel.v.*\.zip$"
file_info.ramdisk        = 'jflte/TouchWiz/TouchWiz.def'
file_info.patch          = 'jflte/Kernels/TouchWiz/infamouskernel.dualboot.patch'

def matches(filename):
  if re.search(filename_regex, filename):
    return True
  else:
    return False

def print_message():
  print("Detected Infamous kernel zip")

def get_file_info():
  return file_info
