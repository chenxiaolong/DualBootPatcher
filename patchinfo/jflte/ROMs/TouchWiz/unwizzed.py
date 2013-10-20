from fileinfo import FileInfo
import re

file_info = FileInfo()

filename_regex           = r"^Evil_UnWizzed_v[0-9]+\.zip$"
file_info.ramdisk        = 'jflte/TouchWiz/ausdim.dualboot.cpio'
file_info.patch          = 'jflte/ROMs/TouchWiz/unwizzed.dualboot.patch'

def matches(filename):
  if re.search(filename_regex, filename):
    return True
  else:
    return False

def print_message():
  print("Detected Evil UnWizzed ROM zip")
  print("Using patched Ausdim ramdisk (compatible with UnWizzed)")

def get_file_info():
  return file_info
