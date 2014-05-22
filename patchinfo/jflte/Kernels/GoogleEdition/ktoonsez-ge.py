from multiboot.fileinfo import FileInfo
import multiboot.autopatcher as autopatcher
import os

file_info = FileInfo()

filename_regex           = r"^KT-SGS4-(JB4.3|KK4.4)-TWGE-.*\.zip$"
file_info.name           = 'Ktoonsez Google Edition kernel'
file_info.ramdisk        = 'jflte/GoogleEdition/GoogleEdition.def'
file_info.patch          = autopatcher.auto_patch
file_info.extract        = autopatcher.files_to_auto_patch

def get_file_info(filename):
  if not filename:
    return file_info

  filename = os.path.split(filename)[1]
  if 'VZW' in filename or 'ATT' in filename:
    file_info.bootimg    = 'boot.lok'
    file_info.loki       = True
  return file_info
