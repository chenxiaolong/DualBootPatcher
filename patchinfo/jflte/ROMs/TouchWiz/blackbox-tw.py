from multiboot.fileinfo import FileInfo
import multiboot.autopatcher as autopatcher
import re

file_info = FileInfo()

filename_regex           = r"^[a-zA-Z0-9]+BlackBox[0-9]+SGS4\.zip$"
file_info.name           = 'BlackBox TouchWiz'
file_info.ramdisk        = 'jflte/TouchWiz/TouchWiz.def'
file_info.patch          = autopatcher.auto_patch
file_info.extract        = autopatcher.files_to_auto_patch
file_info.bootimg        = 'dsa/Kernels/Stock/boot.img'

def matches(filename):
  if re.search(filename_regex, filename):
    return True
  else:
    return False

def get_file_info(filename = ""):
  if re.search(r"BlackBox22", filename):
    file_info.bootimg    = 'dsa/Kernels/KTSGS4/boot.img'
  return file_info
