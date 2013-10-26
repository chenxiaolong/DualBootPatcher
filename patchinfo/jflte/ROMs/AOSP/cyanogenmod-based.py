from fileinfo import FileInfo
import re

file_info = FileInfo()

filename_regex           = r"^cm-[0-9\.]+-[0-9]+-.*.zip$"
file_info.ramdisk        = 'jflte/AOSP/AOSP.def'
file_info.patch          = 'jflte/ROMs/AOSP/cyanogenmod.dualboot.patch'

def matches(filename):
  if re.search(filename_regex, filename):
    return True
  else:
    return False

def print_message():
  print("Detected Cyanogenmod based ROM zip")

def get_file_info(filename = ""):
  # My ROM has built in dual-boot support
  if "noobdev" in filename:
    print("ROM has built in dual boot support")
    return None

  return file_info
