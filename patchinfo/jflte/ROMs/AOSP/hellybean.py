from fileinfo import FileInfo
import re

file_info = FileInfo()

filename_regex           = r"^Hellybean-.*\.zip$"
file_info.ramdisk        = 'jflte/AOSP/cyanogenmod.dualboot.cpio'
file_info.patch          = 'jflte/ROMs/AOSP/cyanogenmod.dualboot.patch'

def matches(filename):
  if re.search(filename_regex, filename):
    return True
  else:
    return False

def print_message():
  print("Detected HellyBean ROM zip")
  print("Using patched Cyanogenmod ramdisk")

def get_file_info():
  return file_info
