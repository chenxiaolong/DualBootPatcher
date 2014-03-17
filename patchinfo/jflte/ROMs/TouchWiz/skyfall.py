from multiboot.fileinfo import FileInfo
import multiboot.autopatcher as autopatcher
import re

file_info = FileInfo()

filename_regex           = r"^SF-.*\.zip$"
file_info.name           = 'SkyFall'
file_info.ramdisk        = 'jflte/TouchWiz/TouchWiz.def'
file_info.patch          = autopatcher.auto_patch
file_info.extract        = autopatcher.files_to_auto_patch
file_info.bootimg        = 'kernel/boot.lok'

def matches(filename):
  if re.search(filename_regex, filename):
    return True
  else:
    return False

def get_file_info():
  return file_info
