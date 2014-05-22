from multiboot.fileinfo import FileInfo
import multiboot.autopatcher as autopatcher
import multiboot.fileio as fileio

file_info = FileInfo()

filename_regex           = r"^[a-zA-Z0-9]+BlackBox[0-9]+SGS4\.zip$"
file_info.name           = 'BlackBox TouchWiz'
file_info.ramdisk        = 'jflte/TouchWiz/TouchWiz.def'
file_info.patch          = autopatcher.auto_patch
file_info.extract        = autopatcher.files_to_auto_patch
file_info.bootimg        = 'dsa/Kernels/Stock/boot.img'

def get_file_info(filename):
  if not filename:
    return file_info

  if fileio.filename_matches(filename, r"BlackBox22"):
    file_info.bootimg    = 'dsa/Kernels/KTSGS4/boot.img'
  return file_info
