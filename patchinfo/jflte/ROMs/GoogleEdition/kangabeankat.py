from multiboot.fileinfo import FileInfo
import multiboot.autopatcher as autopatcher
from multiboot.autopatchers.jflte import GoogleEditionPatcher
import re
import zipfile

file_info = FileInfo()

# Kangabean/Kangakat
filename_regex           = r"^K[BK]-.*\.zip$"
file_info.name           = 'Kangabean/Kangakat'
file_info.ramdisk        = 'jflte/GoogleEdition/GoogleEdition.def'

def get_file_info(filename):
  if not filename:
    return file_info

  # Search zip for boot image
  with zipfile.ZipFile(filename, 'r') as z:
    for name in z.namelist():
      if re.search(r'(^|/)boot.(img|lok)$', name):
        file_info.bootimg = name

  if not file_info.bootimg:
    file_info.bootimg = 'boot.img'

  if file_info.bootimg.endswith('.lok'):
    file_info.loki       = True

  if filename.startswith("KK"):
    file_info.patch      = [ autopatcher.auto_patch, GoogleEditionPatcher.qcom_audio_fix ]
    file_info.extract    = [ autopatcher.files_to_auto_patch, GoogleEditionPatcher.files_for_qcom_audio_fix ]
  elif filename.startswith("KB"):
    file_info.patch      = [ autopatcher.auto_patch ]
    file_info.extract    = [ autopatcher.files_to_auto_patch ]

  return file_info
