from fileinfo import FileInfo
import re

file_info = FileInfo()

filename_regex           = r"CARBON-JB-.*-[a-z0-9]+\.zip"
file_info.ramdisk        = 'jflte/AOSP/carbon.dualboot.cpio'
file_info.patch          = 'jflte/ROMs/AOSP/carbon.dualboot.patch'

def matches(filename):
  if re.search(filename_regex, filename):
    return True
  else:
    return False

def print_message(filename = ""):
  if 'NIGHTLY' in filename:
    print("Detected Carbon Nightly ROM zip")
  else:
    print("Detected Carbon ROM zip")
  print("Using patched Carbon ramdisk")

def get_file_info():
  return file_info
