from multiboot.fileinfo import FileInfo
import multiboot.autopatcher as autopatcher
import os

file_info = FileInfo()

file_info.name           = 'Xposed Framework Disabler'
file_info.patch          = 'Other/xposed.dualboot.patch'
file_info.has_boot_image = False

def matches(filename):
  filename = os.path.split(filename)[1]
  return filename == 'Xposed-Disabler-Recovery.zip'
