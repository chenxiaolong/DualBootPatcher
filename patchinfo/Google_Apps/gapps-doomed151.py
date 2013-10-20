from fileinfo import FileInfo
import os, re

file_info = FileInfo()

filename_regex           = r"^gapps-jb43-[0-9]+-dmd151\.zip$"
file_info.patch          = os.path.join('Google_Apps', 'gapps-doomed151.dualboot.patch')
file_info.has_boot_image = False

def matches(filename):
  if re.search(filename_regex, filename):
    return True
  else:
    return False

def print_message():
  print("Detected doomed151's Google Apps zip")

def get_file_info():
  return file_info
