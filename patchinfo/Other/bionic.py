from fileinfo import FileInfo
import common as c
import re

file_info = FileInfo()

file_info.patch          = c.auto_patch
file_info.extract        = c.files_to_auto_patch
file_info.has_boot_image = False

def matches(filename):
  if filename == '4.4_qc-optimized_bionic.zip' or \
      filename == '4.4_qc-optimized_dalvik.zip' or \
      filename == 'KRT16S_stock_revert.zip':
    return True
  else:
    return False

def print_message():
  print("Detected Moto X's Qualcomm-optimized Dalvik and Bionic libraries zip")

def get_file_info():
  return file_info
