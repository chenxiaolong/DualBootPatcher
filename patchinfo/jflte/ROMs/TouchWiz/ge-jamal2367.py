from fileinfo import FileInfo
import re

file_info = FileInfo()

filename_regex           = r"^I9505_-_Official_Google_Edition_.*Jamal2367.*\.zip$"
file_info.ramdisk        = 'jflte/TouchWiz/googleedition.dualboot.cpio'
file_info.patch          = 'jflte/ROMs/TouchWiz/ge-jamal2367.dualboot.patch'

def matches(filename):
  if re.search(filename_regex, filename):
    return True
  else:
    return False

def print_message():
  print("Detected jamal2367's Google Edition ROM zip")
  print("Using patched Google Edition ramdisk")

def get_file_info():
  return file_info
