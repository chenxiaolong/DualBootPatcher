from multiboot.fileinfo import FileInfo
import multiboot.autopatcher as autopatcher
from multiboot.autopatchers.jflte import GoogleEditionPatcher
import re

file_info = FileInfo()

# Kangabean/Kangakat
filename_regex           = r"^K[BK]-.*\.zip$"
file_info.ramdisk        = 'jflte/GoogleEdition/GoogleEdition.def'
file_info.loki           = True

def print_message():
  print("Detected Kangabean/Kangakat ROM zip")

def get_file_info():
  return file_info

def matches(filename):
  if re.search(filename_regex, filename):
    if filename.startswith("KK"):
      file_info.patch    = [ autopatcher.auto_patch, GoogleEditionPatcher.qcom_audio_fix ]
      file_info.extract  = [ autopatcher.files_to_auto_patch, GoogleEditionPatcher.files_for_qcom_audio_fix ]
      file_info.bootimg  = "kernel/boot.lok"
    elif filename.startswith("KB"):
      file_info.patch    = [ autopatcher.auto_patch ]
      file_info.extract  = [ autopatcher.files_to_auto_patch ]
      file_info.bootimg  = "boot.lok"
    return True
  else:
    return False
