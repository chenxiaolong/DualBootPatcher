from fileinfo import FileInfo
import os, re

file_info = FileInfo()

filename_regex           = r"^.*_AdamKernel.V[0-9\.]+\.CWM\.zip$"
file_info.ramdisk        = os.path.join('jflte', 'TouchWiz', 'adam.dualboot.cpio')
file_info.patch          = os.path.join('jflte', 'Kernels', 'TouchWiz', 'adam.dualboot.patch')
file_info.bootimg        = "wanam/boot.img"

def matches(filename):
  if re.search(filename_regex, filename):
    return True
  else:
    return False

def print_message():
  print("Detected Adam kernel zip")
  print("Using patched Adam kernel ramdisk")

def get_file_info():
  return file_info
