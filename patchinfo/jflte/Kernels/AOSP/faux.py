from multiboot.fileinfo import FileInfo
import multiboot.autopatcher as autopatcher
import os

file_info = FileInfo()

filename_regex           = r"^jflte[a-z]+-aosp-faux123-.*\.zip$"
file_info.name           = 'Faux kernel'
file_info.ramdisk        = 'jflte/AOSP/AOSP.def'
file_info.patch          = autopatcher.auto_patch
file_info.extract        = autopatcher.files_to_auto_patch

def get_file_info(filename):
  if not filename:
    return file_info

  filename = os.path.split(filename)[1]
  if 'vzw' in filename or 'att' in filename:
    file_info.bootimg    = 'boot.lok'
    file_info.loki       = True
  return file_info
