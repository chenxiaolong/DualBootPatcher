from fileinfo import FileInfo
import re

file_info = FileInfo()

filename_regex           = r"^negalite-.*\.zip"
file_info.ramdisk        = 'jflte/TouchWiz/TouchWiz.def'
file_info.patch          = 'jflte/ROMs/TouchWiz/negalite.dualboot.patch'
file_info.bootimg        = "kernel/stock_kernel/kernel/boot.img"

def matches(filename):
  if re.search(filename_regex, filename):
    return True
  else:
    return False

def print_message():
  print("Detected Negalite ROM zip")

def get_file_info():
  return file_info
