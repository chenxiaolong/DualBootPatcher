from fileinfo import FileInfo
import os, re

file_info = FileInfo()

filename_regex           = r"^pac_.*-Black-Power-Edition_[0-9]+\.zip$"
file_info.ramdisk        = os.path.join('jflte', 'AOSP', 'ktoonsez.dualboot.cpio')
file_info.patch          = os.path.join('jflte', 'ROMs', 'AOSP', 'pacman-metaiiica.dualboot.patch')

def matches(filename):
  if re.search(filename_regex, filename):
    return True
  else:
    return False

def print_message():
  print("Detected Metaiiica's PAC-Man ROM zip")
  print("Using patched KToonsez ramdisk (identical to PAC-Man's ramdisk)")

def get_file_info():
  return file_info
