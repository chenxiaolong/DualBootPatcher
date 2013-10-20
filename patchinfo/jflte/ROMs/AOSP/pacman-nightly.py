from fileinfo import FileInfo
import os, re

file_info = FileInfo()

filename_regex           = r"^pac_[a-z0-9]+-nightly-[0-9]+.zip$"
file_info.ramdisk        = os.path.join('jflte', 'AOSP', 'pacman.dualboot.cpio')
file_info.patch          = os.path.join('jflte', 'ROMs', 'AOSP', 'cyanogenmod.dualboot.patch')

def matches(filename):
  if re.search(filename_regex, filename):
    return True
  else:
    return False

def print_message():
  print("Detected PAC-Man nightly ROM zip")
  print("Using patched PAC-Man ramdisk")

def get_file_info():
  return file_info
