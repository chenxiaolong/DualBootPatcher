from fileinfo import FileInfo
import re

file_info = FileInfo()

filename_regex           = r"^negalite-wonderom-r[0-9]+\.zip"
file_info.ramdisk        = 'jflte/TouchWiz/touchwiz.dualboot.cpio'
file_info.patch          = 'jflte/ROMs/TouchWiz/negalite.dualboot.patch'
file_info.bootimg        = "kernel/stock_kernel/kernel/boot.img"

def matches(filename):
  if re.search(filename_regex, filename):
    return True
  else:
    return False

def print_message():
  print("Detected Negalite ROM zip")
  print("Using patched TouchWiz ramdisk")

def get_file_info():
  return file_info
