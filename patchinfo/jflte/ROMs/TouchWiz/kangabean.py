from fileinfo import FileInfo
import os, re

file_info = FileInfo()

filename_regex           = r"KB-.*\.zip$"
#file_info.ramdisk        = "jflte/TouchWiz/googleedition.dualboot.cpio"
file_info.ramdisk        = os.path.join('jflte', 'TouchWiz', 'kangabean.dualboot.patch')
file_info.patch          = os.path.join('jflte', 'ROMs', 'TouchWiz', 'ge-jamal2367.dualboot.patch')
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
  print("Using patched Google Edition ramdisk")

def get_file_info():
  return file_info
