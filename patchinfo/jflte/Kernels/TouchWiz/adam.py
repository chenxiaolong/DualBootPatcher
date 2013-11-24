from fileinfo import FileInfo
import common as c
import re

file_info = FileInfo()

filename_regex           = r"^.*_AdamKernel.V[0-9\.]+\.CWM\.zip$"
file_info.ramdisk        = 'jflte/TouchWiz/TouchWiz.def'
file_info.patch          = c.auto_patch
file_info.extract        = c.files_to_auto_patch
file_info.bootimg        = "wanam/boot.img"

def matches(filename):
  if re.search(filename_regex, filename):
    return True
  else:
    return False

def print_message():
  print("Detected Adam kernel zip")

def get_file_info():
  return file_info
