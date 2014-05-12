from multiboot.fileinfo import FileInfo
import multiboot.autopatcher as autopatcher
import multiboot.fileio as fileio
import re

file_info = FileInfo()

filename_regex           = r"^cm-.*noobdev.*.zip$"
file_info.name           = "chenxiaolong's noobdev CyanogenMod"
file_info.ramdisk        = 'hammerhead/AOSP/cxl.def'
file_info.patch          = [ autopatcher.auto_patch ]
file_info.extract        = [ autopatcher.files_to_auto_patch, 'system/build.prop' ]
# ROM has built in dual boot support
file_info.configs        = [ 'all', '!dualboot' ]

def matches(filename):
  if re.search(filename_regex, filename):
    return True
  else:
    return False

def get_file_info(filename = ""):
  return file_info

# My ROM has dual-boot support built in, so we'll hack around that to support
# triple, quadruple, ... boot
def multi_boot(directory, bootimg = None, device_check = True,
               partition_config = None, device = None):
  updater_script = 'META-INF/com/google/android/updater-script'
  lines = fileio.all_lines(updater_script, directory = directory)

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

  fileio.write_lines(updater_script, lines, directory = directory)

  # Create /tmp/dualboot.prop
  lines = fileio.all_lines('dualboot.sh', directory = directory)

  lines.append("echo 'ro.dualboot=0' > /tmp/dualboot.prop")

  fileio.write_lines('dualboot.sh', lines, directory = directory)

file_info.patch.insert(0, multi_boot)

# The auto-updater in my ROM needs to know if the ROM has been patched
def system_prop(directory, bootimg = None, device_check = True,
                partition_config = None, device = None):
  lines = fileio.all_lines('system/build.prop', directory = directory)

  lines.append('ro.chenxiaolong.patched=%s\n' % partition_config.id)

  fileio.write_lines('system/build.prop', lines, directory = directory)

file_info.patch.append(system_prop)
