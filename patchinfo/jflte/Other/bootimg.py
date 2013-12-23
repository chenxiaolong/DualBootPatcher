from multiboot.fileinfo import FileInfo
import re

file_info = FileInfo()

filename_regex           = r"^.*\.img$"
file_info.ramdisk        = 'jflte/AOSP/AOSP.def'

def matches(filename):
  if re.search(filename_regex, filename):
    return True
  else:
    return False

def print_message():
  print("Detected boot.img file")
  print("ASSUMING RAMDISK IS FOR AOSP. USE patchramdisk SCRIPT IF IT'S FOR TOUCHWIZ")

def get_file_info():
  return file_info
