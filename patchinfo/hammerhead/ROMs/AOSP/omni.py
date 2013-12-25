from multiboot.fileinfo import FileInfo
import multiboot.autopatcher as autopatcher
import re

file_info = FileInfo()

filename_regex           = r"^omni-[0-9\.]+-[0-9]+.*\.zip$"
file_info.ramdisk        = 'hammerhead/AOSP/AOSP.def'
file_info.patch          = autopatcher.auto_patch
file_info.extract        = autopatcher.files_to_auto_patch

def matches(filename):
  if re.search(filename_regex, filename):
    return True
  else:
    return False

def print_message():
  print("Detected official Omni ROM zip")

def get_file_info():
  return file_info
