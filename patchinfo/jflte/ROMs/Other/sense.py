from fileinfo import FileInfo
import common as c

file_info = FileInfo()

file_info.ramdisk        = 'jflte/AOSP/AOSP.def'
file_info.patch          = c.auto_patch
file_info.extract        = c.files_to_auto_patch

def matches(filename):
  if filename.lower() == "sense5port.zip" or \
     filename         == "4.3 official port.zip" or \
     filename         == "ALPHA V1.5.zip":
    return True
  else:
    return False

def print_message():
  print("Detected geiti94's HTC Sense 5 port zip")

def get_file_info():
  return file_info
