from fileinfo import FileInfo
import os, re

file_info = FileInfo()

filename_regex           = r"^i9505-ge-untouched-4.3-.*.zip$"
file_info.ramdisk        = os.path.join('jflte', 'TouchWiz', 'googleedition.dualboot.cpio')
file_info.patch          = os.path.join('jflte', 'ROMs', 'TouchWiz', 'ge-MaKTaiL.dualboot.patch')

def matches(filename):
  if re.search(filename_regex, filename):
    return True
  else:
    return False

def print_message():
  print("Detected MaKTaiL's Google Edition ROM zip")
  print("Using patched Google Edition ramdisk")

def get_file_info():
  return file_info
