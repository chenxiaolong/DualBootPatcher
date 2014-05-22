from multiboot.fileinfo import FileInfo
import multiboot.autopatcher as autopatcher
import os

file_info = FileInfo()

filename_regex           = r"^KT-SGS4-[^-]+-AOSP-.*\.zip$"
file_info.name           = 'Ktoonsez AOSP kernel'
file_info.ramdisk        = 'jflte/AOSP/ktoonsez.def'
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
