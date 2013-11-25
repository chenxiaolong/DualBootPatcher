from fileinfo import FileInfo
import common as c
import re

file_info = FileInfo()

filename_regex           = r"^GE.*\.zip$"
file_info.ramdisk        = 'jflte/TouchWiz/TouchWiz.def'
file_info.patch          = c.auto_patch
file_info.extract        = c.files_to_auto_patch
file_info.bootimg        = 'kernel/boot.img'

def print_message():
  print("Detected Goldeneye")

def matches(filename):
  if re.search(filename_regex, filename):
    if 'ATT' in filename:
      file_info.loki     = True
      file_info.bootimg  = 'kernel/boot.lok'
    return True
  else:
    return False

def get_file_info():
  return file_info
