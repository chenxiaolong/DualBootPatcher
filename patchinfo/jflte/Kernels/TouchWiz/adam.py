from fileinfo import FileInfo
import re

file_info = FileInfo()

filename_regex           = r"^.*_AdamKernel.V[0-9\.]+\.CWM\.zip$"
file_info.ramdisk        = 'jflte/TouchWiz/TouchWiz.def'
file_info.patch          = 'jflte/Kernels/TouchWiz/adam.dualboot.patch'
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
