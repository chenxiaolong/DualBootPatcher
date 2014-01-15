from multiboot.fileinfo import FileInfo
import re

file_info = FileInfo()

filename_regex           = r"^ComaDose_V[0-9\.]+_Cossbreeder_[0-9\.]+\.zip"
file_info.name           = 'ComaDose'
file_info.patch          = 'jflte/Other/comadose.dualboot.patch'
file_info.has_boot_image = False

def matches(filename):
  if re.search(filename_regex, filename):
    return True
  else:
    return False

def get_file_info():
  return file_info
