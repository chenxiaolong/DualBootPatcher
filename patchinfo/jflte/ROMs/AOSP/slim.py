from fileinfo import FileInfo
import re

file_info = FileInfo()

filename_regex           = r"^Slim-.*.zip$"
file_info.ramdisk        = 'jflte/AOSP/AOSP.def'
file_info.patch          = 'jflte/ROMs/AOSP/slim.dualboot.patch'

def matches(filename):
  if re.search(filename_regex, filename):
    if 'jfltevzw' in filename or 'jflteatt' in filename:
      file_info.patch    = 'jflte/ROMs/AOSP/slim_loki.dualboot.patch'
    return True
  else:
    return False

def print_message():
  print("Detected Slim Bean ROM zip")

def get_file_info():
  return file_info
