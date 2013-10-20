from fileinfo import FileInfo
import re

file_info = FileInfo()

filename_regex           = r"^Slim-.*.zip$"
file_info.ramdisk        = 'jflte/AOSP/cyanogenmod.dualboot.cpio'
file_info.patch          = 'jflte/ROMs/AOSP/slim.dualboot.patch'

def matches(filename):
  if re.search(filename_regex, filename):
    return True
  else:
    return False

def print_message():
  print("Detected Slim Bean ROM zip")
  print("Using patched Cyanogenmod ramdisk (compatible with Slim Bean)")

def get_file_info():
  return file_info
