from multiboot.fileinfo import FileInfo
import multiboot.autopatcher as autopatcher
import multiboot.fileio as fileio
from multiboot.autopatchers.jflte import GoogleEditionPatcher
import os

file_info = FileInfo()

filename_regex           = r"^I9505_-_Official_Google_Edition_.*Jamal2367.*\.zip$"
file_info.name           = "jamal2367's Google Edition"
file_info.ramdisk        = 'jflte/GoogleEdition/GoogleEdition.def'
file_info.patch          = [ autopatcher.auto_patch, GoogleEditionPatcher.qcom_audio_fix ]
file_info.extract        = [ autopatcher.files_to_auto_patch, GoogleEditionPatcher.files_for_qcom_audio_fix ]

def matches(filename):
  return fileio.filename_matches(filename, filename_regex) or \
    filename == 'I9505_-_Google_Edition_v6_by_Jamal2367.zip'

def get_file_info(filename):
  if not filename:
    return file_info

  filename = os.path.split(filename)[1]
  if filename == 'I9505_-_Google_Edition_v6_by_Jamal2367.zip':
    # A bit hackish, but it works
    file_info.ramdisk    = 'jflte/AOSP/AOSP.def'
    file_info.patch      = [ autopatcher.auto_patch ]
    file_info.extract    = [ autopatcher.files_to_auto_patch ]
  return file_info
