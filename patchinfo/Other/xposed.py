from multiboot.fileinfo import FileInfo
import multiboot.autopatcher as autopatcher
import re

file_info = FileInfo()

file_info.name           = 'Xposed Framework Disabler'
file_info.patch          = 'Other/xposed.dualboot.patch'
file_info.has_boot_image = False

def matches(filename):
  if filename == 'Xposed-Disabler-Recovery.zip':
    return True
  else:
    return False

def get_file_info():
  return file_info
