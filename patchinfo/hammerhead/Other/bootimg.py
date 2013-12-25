from multiboot.fileinfo import FileInfo
import re

file_info = FileInfo()

filename_regex           = r"^.*\.img$"
file_info.ramdisk        = 'hammerhead/AOSP/AOSP.def'

def matches(filename):
  if re.search(filename_regex, filename):
    return True
  else:
    return False

def print_message():
  print("Detected boot.img file")

def get_file_info():
  return file_info
