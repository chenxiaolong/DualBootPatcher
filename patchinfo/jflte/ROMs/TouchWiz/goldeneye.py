from multiboot.fileinfo import FileInfo
import multiboot.autopatcher as autopatcher
import re

file_info = FileInfo()

att_filenames = [
  'GE-XXUEML6-18.2.zip'
]

filename_regex           = r"^GE.*\.zip$"
file_info.name           = 'GoldenEye'
file_info.ramdisk        = 'jflte/TouchWiz/TouchWiz.def'
file_info.patch          = autopatcher.auto_patch
file_info.extract        = autopatcher.files_to_auto_patch
file_info.bootimg        = 'kernel/boot.img'

def matches(filename):
  if re.search(filename_regex, filename):
    if re.search(r'(ATT|[iI]337)', filename) or filename in att_filenames:
      file_info.loki     = True
      file_info.bootimg  = 'kernel/boot.lok'
    return True
  else:
    return False

def get_file_info():
  return file_info
