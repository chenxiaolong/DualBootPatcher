from fileinfo import FileInfo
import common as c
import re

file_info = FileInfo()

filename_regex           = r"^I9505_-_Official_Google_Edition_.*Jamal2367.*\.zip$"
file_info.ramdisk        = 'jflte/GoogleEdition/GoogleEdition.def'
#file_info.patch          = 'jflte/ROMs/TouchWiz/ge-jamal2367.dualboot.patch'
file_info.patch          = c.auto_patch
file_info.extract        = c.files_to_auto_patch

def matches(filename):
  if re.search(filename_regex, filename):
    return True
  else:
    return False

def print_message():
  print("Detected jamal2367's Google Edition ROM zip")

def get_file_info():
  return file_info
