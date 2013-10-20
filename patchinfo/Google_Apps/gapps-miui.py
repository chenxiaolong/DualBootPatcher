from fileinfo import FileInfo
import os, re

file_info = FileInfo()

filename_regex           = r"^miuiandroid_gapps.*\.zip$"
file_info.patch          = os.path.join('GoogleApps', 'gapps-miui.dualboot.patch')
file_info.has_boot_image = False

def matches(filename):
  if re.search(filename_regex, filename):
    return True
  else:
    return False

def print_message():
  print("Detected MIUI Google Apps zip")

def get_file_info():
  return file_info
