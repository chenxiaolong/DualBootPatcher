from fileinfo import FileInfo
import re

file_info = FileInfo()

filename_regex           = r"KB-.*\.zip$"
#file_info.ramdisk        = 'jflte/GoogleEdition/GoogleEdition.def'
file_info.patch          = 'jflte/ROMs/TouchWiz/ge-jamal2367.dualboot.patch'
file_info.loki           = True
#file_info.bootimg        = "boot.lok"
file_info.has_boot_image = False

def matches(filename):
  if re.search(filename_regex, filename):
    return True
  else:
    return False

def print_message():
  print("Detected Kangabean ROM zip")

def get_file_info():
  return file_info
