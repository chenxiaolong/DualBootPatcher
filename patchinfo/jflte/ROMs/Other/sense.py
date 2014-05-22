from multiboot.fileinfo import FileInfo
import multiboot.autopatcher as autopatcher
import os

file_info = FileInfo()

file_info.name           = "geiti94's HTC Sense 5 port"
file_info.ramdisk        = 'jflte/AOSP/AOSP.def'
file_info.patch          = autopatcher.auto_patch
file_info.extract        = autopatcher.files_to_auto_patch

def matches(filename):
  filename = os.path.split(filename)[1]
  return filename.lower() == "sense5port.zip" or \
         filename         == "4.3 official port.zip" or \
         filename         == "ALPHA V1.5.zip"
