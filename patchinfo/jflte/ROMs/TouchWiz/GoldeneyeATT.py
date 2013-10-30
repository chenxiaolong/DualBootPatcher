from fileinfo import FileInfo
import re

file_info = FileInfo()

filename_regex           = r"^GE-XXUE.*\.zip$"
file_info.ramdisk        = 'jflte/AOSP/TouchWiz.def'
file_info.has_boot_image = True
file_info.patch          = 'jflte/AOSP/GoldeneyeATT.patch'
file_info.bootimg        = 'kernel/boot.lok'

def print_message():
  print("Detected Goldeneye - ATT")

###

def matches(filename):
  if re.search(filename_regex, filename):
    return True
  else:
    return False

def get_file_info():
  return file_info
