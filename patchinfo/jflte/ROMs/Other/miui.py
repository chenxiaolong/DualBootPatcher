from fileinfo import FileInfo
import os, re

file_info = FileInfo()

filename_regex           = r"^miuiandroid_jflte.*\.zip$"
file_info.ramdisk        = os.path.join('jflte', 'Other', 'miui.dualboot.cpio')
file_info.patch          = os.path.join('jflte', 'ROMs', 'Other', 'miui.dualboot.patch')

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
