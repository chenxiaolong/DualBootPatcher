from fileinfo import FileInfo
import common as c
import re

file_info = FileInfo()

filename_regex           = r"^cm-[0-9\.]+-[0-9]+-.*.zip$"
file_info.ramdisk        = 'jflte/AOSP/AOSP.def'

def matches(filename):
  if re.search(filename_regex, filename):
    return True
  else:
    return False

def print_message():
  print("Detected Cyanogenmod based ROM zip")

def get_file_info(filename = ""):
  # My ROM has built in dual-boot support
  if "noobdev" in filename:
    print("ROM has built in dual boot support")
    return None

  return file_info

def extract_files():
  return [ 'META-INF/com/google/android/updater-script' ]

def patch_files(directory):
  lines = c.get_lines_from_file(directory, 'META-INF/com/google/android/updater-script')

  c.attempt_auto_patch(lines, bootimg = file_info.bootimg)

  c.write_lines_to_file(directory, 'META-INF/com/google/android/updater-script', lines)
