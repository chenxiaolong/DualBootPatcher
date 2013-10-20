from fileinfo import FileInfo
import re

file_info = FileInfo()

filename_regex           = r"^Liquid-JB-v[0-9\.]+-OFFICIAL-.*\.zip$"
file_info.ramdisk        = 'jflte/AOSP/liquidsmooth.dualboot.cpio'
file_info.patch          = 'jflte/ROMs/AOSP/liquidsmooth.dualboot.patch'

def matches(filename):
  if re.search(filename_regex, filename):
    return True
  else:
    return False

def print_message():
  print("Detected Official LiquidSmooth ROM zip")
  print("Using patched LiquidSmooth ramdisk")

def get_file_info():
  return file_info
