from fileinfo import FileInfo
import common as c
import re

file_info = FileInfo()

filename_regex           = r"^MODIFIED_STOCK_.*\.zip$"
file_info.ramdisk        = 'jflte/TouchWiz/TouchWiz.def'
file_info.patch          = c.auto_patch
file_info.extract        = c.files_to_auto_patch
file_info.bootimg        = 'boot.img'

def print_message():
  print("Detected Omnifarious's modified stock TouchWiz ROM")

def matches(filename):
  if re.search(filename_regex, filename):
    return True
  else:
    return False

def get_file_info():
  return file_info
