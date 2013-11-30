from fileinfo import FileInfo
import common as c
import re

file_info = FileInfo()

filename_regex           = r"^cm-.*noobdev.*.zip$"
file_info.ramdisk        = 'jflte/AOSP/cxl.def'
file_info.patch          = [ c.auto_patch ]
file_info.extract        = [ c.files_to_auto_patch ]
# ROM has built in dual boot support
file_info.configs        = [ 'all', '!dualboot' ]

def matches(filename):
  if re.search(filename_regex, filename):
    return True
  else:
    return False

def print_message():
  print("Detected chenxiaolong's noobdev CyanogenMod ROM zip")

def get_file_info(filename = ""):
  return file_info

# My ROM has dual-boot support built in, so we'll hack around that to support
# triple, quadruple, ... boot
def multi_boot(directory, bootimg = None, device_check = True,
               partition_config = None):
  lines = c.get_lines_from_file(directory, 'META-INF/com/google/android/updater-script')

  i = 0
  while i < len(lines):
    # Remove existing dualboot.sh lines
    if 'system/bin/dualboot.sh' in lines[i]:
      del lines[i]

    # Remove confusing messages
    elif 'boot installation is' in lines[i]:
      # Can't remove - sole line inside if statement
      lines[i] = 'ui_print("");\n'
      i += 1

    elif 'set-secondary' in lines[i]:
      lines[i] = 'ui_print("");\n'
      i += 1

    else:
      i += 1

  c.write_lines_to_file(directory, 'META-INF/com/google/android/updater-script', lines)

  # Create /tmp/dualboot.prop
  lines = c.get_lines_from_file(directory, 'dualboot.sh')

  lines.append("echo 'ro.dualboot=0' > /tmp/dualboot.prop")

  c.write_lines_to_file(directory, 'dualboot.sh', lines)

file_info.patch.insert(0, multi_boot)
