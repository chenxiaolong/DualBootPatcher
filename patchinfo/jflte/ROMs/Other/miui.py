from fileinfo import FileInfo
import re

file_info = FileInfo()

filename_regex           = r"^miuiandroid_jflte.*\.zip$"
file_info.ramdisk        = 'jflte/AOSP/AOSP.def'
file_info.patch          = 'jflte/ROMs/Other/miui.dualboot.patch'
file_info.need_new_init  = True

def matches(filename):
  if re.search(filename_regex, filename):
    return True
  else:
    return False

def print_message():
  print("Detected MIUI ROM zip")

def get_file_info():
  return file_info
