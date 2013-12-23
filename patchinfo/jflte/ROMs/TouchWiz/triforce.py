from multiboot.fileinfo import FileInfo
import multiboot.autopatcher as autopatcher
import multiboot.fileio as fileio
import re

file_info = FileInfo()

filename_regex           = r"^TriForceROM[0-9\.]+\.zip$"
file_info.ramdisk        = 'jflte/TouchWiz/TouchWiz.def'
file_info.patch          = [ autopatcher.auto_patch ]
file_info.extract        = [ autopatcher.files_to_auto_patch,
                             'META-INF/com/google/android/aroma-config' ]
file_info.bootimg        = "aroma/kernels/stock/boot.img"

def matches(filename):
  if re.search(filename_regex, filename):
    return True
  else:
    return False

def print_message():
  print("Detected TriForceROM zip")

def get_file_info():
  return file_info

def fix_aroma(directory, bootimg = None, device_check = True,
              partition_config = None):
  aroma_config = 'META-INF/com/google/android/aroma-config'
  lines = fileio.all_lines(aroma_config, directory = directory)

  i = 0
  while i < len(lines):
    if re.search('/system/build.prop', lines[i]):
      # Remove 'raw-' since aroma mounts the partitions directly
      target_dir = re.sub("raw-", "", partition_config.target_system)
      lines[i] = re.sub('/system', target_dir, lines[i])

    elif re.search(r"/sbin/mount.*/system", lines[i]):
      i += autopatcher.insert_line(i + 1, re.sub('/system', '/cache', lines[i]), lines)
      i += autopatcher.insert_line(i + 1, re.sub('/system', '/data', lines[i]), lines)

    elif '~welcome.title' in lines[i]:
      i += autopatcher.insert_line(i + 1, '"***IMPORTANT***: You MUST choose the stock kernel for dual booting to work. If you want to use a custom kernel, you can patch and flash it afterwards.\\n\\n" +', lines)

    i += 1

  fileio.write_lines(aroma_config, lines, directory = directory)

file_info.patch.append(fix_aroma)
