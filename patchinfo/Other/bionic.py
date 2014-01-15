from multiboot.fileinfo import FileInfo
import multiboot.autopatcher as autopatcher
import re

file_info = FileInfo()

file_info.name           = "Moto X's Qualcomm-optimized Dalvik and Bionic libraries"
file_info.patch          = autopatcher.auto_patch
file_info.extract        = autopatcher.files_to_auto_patch
file_info.has_boot_image = False

def matches(filename):
  if filename == '4.4_qc-optimized_bionic.zip' or \
      filename == '4.4_qc-optimized_dalvik.zip' or \
      filename == 'KRT16S_stock_revert.zip':
    return True
  else:
    return False

def get_file_info():
  return file_info
