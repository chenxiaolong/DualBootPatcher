from multiboot.fileinfo import FileInfo
import os

file_info = FileInfo()

filename_regex           = r"^ChronicKernel-(JB4.3|KK4.4)-AOSP-.*\.zip$"
file_info.name           = 'ChronicKernel'
file_info.ramdisk        = 'jflte/AOSP/AOSP.def'
file_info.patch          = 'jflte/Kernels/AOSP/chronickernel.dualboot.patch'

def get_file_info(filename):
  if not filename:
    return file_info

  filename = os.path.split(filename)[1]
  if 'VZW' in filename or 'ATT' in filename:
    file_info.loki       = True
    file_info.bootimg    = 'boot.lok'
    file_info.patch      = 'jflte/Kernels/AOSP/chronickernel-loki.dualboot.patch'
  return file_info
