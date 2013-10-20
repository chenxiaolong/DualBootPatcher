from fileinfo import FileInfo
import os, re

file_info = FileInfo()

filename_regex           = r"^Liquid-JB-v[0-9\.]+-OFFICIAL-.*\.zip$"
file_info.ramdisk        = os.path.join('jflte', 'AOSP', 'liquidsmooth.dualboot.cpio')
file_info.patch          = os.path.join('jflte', 'ROMs', 'AOSP', 'liquidsmooth.dualboot.patch')

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
