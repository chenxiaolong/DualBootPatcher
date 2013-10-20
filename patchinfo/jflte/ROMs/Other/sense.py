from fileinfo import FileInfo
import os

file_info = FileInfo()

file_info.ramdisk        = os.path.join('jflte', 'Other', 'sense.dualboot.cpio')
file_info.patch          = os.path.join('jflte', 'ROMs', 'Other', 'sense-geiti94.dualboot.patch')

def matches(filename):
  if filename.lower() == "sense5port.zip" or \
     filename         == "4.3 official port.zip":
    return True
  else:
    return False

def print_message():
  print("Detected geiti94's HTC Sense 5 port zip")
  print("Using patched Sense ramdisk")

def get_file_info():
  return file_info
