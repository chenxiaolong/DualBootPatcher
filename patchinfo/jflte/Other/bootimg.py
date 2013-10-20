from fileinfo import FileInfo
import re

file_info = FileInfo()

filename_regex           = r"^.*\.img$"
file_info.ramdisk        = 'jflte/AOSP/cyanogenmod.dualboot.cpio'

def matches(filename):
  if re.search(filename_regex, filename):
    return True
  else:
    return False

def print_message():
  print("Detected boot.img file")
  print("WILL USE CYANOGENMOD RAMDISK. USE replaceramdisk SCRIPT TO CHOOSE ANOTHER RAMDISK")

def get_file_info():
  return file_info
