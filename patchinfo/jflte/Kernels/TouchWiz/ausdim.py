from multiboot.fileinfo import FileInfo
import multiboot.autopatcher as autopatcher
import re

file_info = FileInfo()

filename_regex           = r"^v[0-9\.]+-(TW|Google)-[Ee]dition-ausdim-Kernel-.*\.zip$"
file_info.ramdisk        = 'jflte/TouchWiz/TouchWiz.def'
file_info.patch          = autopatcher.auto_patch
file_info.extract        = autopatcher.files_to_auto_patch

def matches(filename):
  if re.search(filename_regex, filename):
    return True
  else:
    return False

def print_message():
  print("Detected Ausdim kernel zip")

def get_file_info():
  return file_info
