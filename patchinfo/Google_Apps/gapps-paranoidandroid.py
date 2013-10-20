from fileinfo import FileInfo
import re

file_info = FileInfo()

filename_regex           = r"^pa_gapps-full-4\.3-[0-9]+-signed\.zip$"
file_info.patch          = 'Google_Apps/gapps-paranoidandroid.patch'
file_info.has_boot_image = False

def matches(filename):
  if re.search(filename_regex, filename):
    return True
  else:
    return False

def print_message():
  print("Detected Paranoid Android Google Apps zip")

def get_file_info():
  return file_info
