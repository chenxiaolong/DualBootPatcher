from fileinfo import FileInfo
import re

file_info = FileInfo()

filename_regex           = r"^i9505-ge-untouched-4.3-.*.zip$"
file_info.ramdisk        = 'jflte/TouchWiz/googleedition.dualboot.cpio'
file_info.patch          = 'jflte/ROMs/TouchWiz/ge-MaKTaiL.dualboot.patch'

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
