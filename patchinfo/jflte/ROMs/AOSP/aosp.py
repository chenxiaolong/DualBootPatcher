from fileinfo import FileInfo
import os, re

file_info = FileInfo()

filename_regex           = r"^aosp43-i9505-.*\.zip$"
file_info.ramdisk        = os.path.join('jflte', 'AOSP', 'aosp.dualboot.cpio')
file_info.patch          = os.path.join('jflte', 'ROMs', 'AOSP', 'aosp.dualboot.patch')

def matches(filename):
  if re.search(filename_regex, filename):
    return True
  else:
    return False

def print_message():
  print("Detected Broodplank's AOSP ROM zip")
  print("Using patched AOSP ramdisk")

def get_file_info():
  return file_info
