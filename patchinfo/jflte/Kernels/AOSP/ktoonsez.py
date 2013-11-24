from fileinfo import FileInfo
import common as c
import re

file_info = FileInfo()

filename_regex           = r"^KT-SGS4-[^-]+-AOSP-.*\.zip$"
file_info.ramdisk        = 'jflte/AOSP/AOSP.def'
file_info.patch          = c.auto_patch
file_info.extract        = c.files_to_auto_patch

def matches(filename):
  if re.search(filename_regex, filename):
    if 'VZW' in filename or 'ATT' in filename:
      file_info.bootimg  = 'boot.lok'
      file_info.loki     = True
    return True
  else:
    return False

def print_message():
  print("Detected ktoonsez kernel zip")

def get_file_info():
  return file_info
