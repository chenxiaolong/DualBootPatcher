from multiboot.fileinfo import FileInfo
import multiboot.autopatcher as autopatcher
import re

file_info = FileInfo()

filename_regex           = r"^jflte[a-z]+-aosp-faux123-.*\.zip$"
file_info.ramdisk        = 'jflte/AOSP/AOSP.def'
file_info.patch          = autopatcher.auto_patch
file_info.extract        = autopatcher.files_to_auto_patch

def matches(filename):
  if re.search(filename_regex, filename):
    if 'vzw' in filename or 'att' in filename:
      file_info.bootimg  = 'boot.lok'
      file_info.loki     = True
    return True
  else:
    return False

def print_message():
  print("Detected faux kernel zip")

def get_file_info():
  return file_info
