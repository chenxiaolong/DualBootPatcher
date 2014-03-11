from multiboot.fileinfo import FileInfo
import multiboot.autopatcher as autopatcher
import re

file_info = FileInfo()

filename_regex           = r"^KT-SGS4-(JB|KK)4.[2-4]-TW-.*\.zip$"
file_info.name           = 'Ktoonsez TouchWiz kernel'
file_info.ramdisk        = 'jflte/TouchWiz/TouchWiz.def'
file_info.patch          = autopatcher.auto_patch
file_info.extract        = autopatcher.files_to_auto_patch

def matches(filename):
  if re.search(filename_regex, filename):
    if 'VZW' in filename or 'ATT' in filename:
      file_info.bootimg  = 'boot.lok'
      file_info.loki     = True
    return True
  else:
    return False

def get_file_info():
  return file_info
