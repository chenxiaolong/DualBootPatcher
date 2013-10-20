from fileinfo import FileInfo
import re

file_info = FileInfo()

filename_regex           = r"^pac_[a-z0-9]+-nightly-[0-9]+.zip$"
file_info.ramdisk        = 'jflte/AOSP/pacman.dualboot.cpio'
file_info.patch          = 'jflte/ROMs/AOSP/cyanogenmod.dualboot.patch'

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
