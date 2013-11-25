from fileinfo import FileInfo
import common as c
import re

file_info = FileInfo()

filename_regex           = r"KB-.*\.zip$"
file_info.ramdisk        = 'jflte/GoogleEdition/GoogleEdition.def'
file_info.patch          = c.auto_patch
file_info.extract        = c.files_to_auto_patch
file_info.bootimg        = "boot.lok"
file_info.loki           = True

def matches(filename):
  if re.search(filename_regex, filename):
    return True
  else:
    return False

def print_message():
  print("Detected Kangabean ROM zip")

def get_file_info():
  return file_info
