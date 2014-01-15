from multiboot.fileinfo import FileInfo
import re

file_info = FileInfo()

filename_regex           = r"^ChronicKernel-(JB4.3|KK4.4)-AOSP-.*\.zip$"
file_info.name           = 'ChronicKernel'
file_info.ramdisk        = 'jflte/AOSP/AOSP.def'
file_info.patch          = 'jflte/Kernels/AOSP/chronickernel.dualboot.patch'

def matches(filename):
  if re.search(filename_regex, filename):
    if 'VZW' in filename or 'ATT' in filename:
      file_info.loki     = True
      file_info.bootimg  = 'boot.lok'
      file_info.patch    = 'jflte/Kernels/AOSP/chronickernel-loki.dualboot.patch'
    return True
  else:
    return False

def get_file_info():
  return file_info
