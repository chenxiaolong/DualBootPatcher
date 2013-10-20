from fileinfo import FileInfo
import re

file_info = FileInfo()

filename_regex           = r"^miuiandroid_jflte.*\.zip$"
file_info.ramdisk        = 'jflte/Other/miui.dualboot.cpio'
file_info.patch          = 'jflte/ROMs/Other/miui.dualboot.patch'

def matches(filename):
  if re.search(filename_regex, filename):
    return True
  else:
    return False

def print_message():
  print("Detected MIUI ROM zip")
  print("Using patched MIUI ramdisk")

def get_file_info():
  return file_info
