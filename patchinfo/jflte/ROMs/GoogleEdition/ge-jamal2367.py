from multiboot.patchinfo import PatchInfo
import multiboot.autopatcher as autopatcher
import multiboot.fileio as fileio
from multiboot.autopatchers.jflte import GoogleEditionPatcher
import os

patchinfo = PatchInfo()

patchinfo.matches        = r"^I9505_-_Official_Google_Edition_.*Jamal2367.*\.zip$"
patchinfo.name           = "jamal2367's Google Edition"
patchinfo.ramdisk        = 'jflte/GoogleEdition/GoogleEdition.def'
patchinfo.patch          = [ autopatcher.auto_patch, GoogleEditionPatcher.qcom_audio_fix ]
patchinfo.extract        = [ autopatcher.files_to_auto_patch, GoogleEditionPatcher.files_for_qcom_audio_fix ]

def on_filename_set(patchinfo, filename):
  filename = os.path.split(filename)[1]
  if filename == 'I9505_-_Google_Edition_v6_by_Jamal2367.zip':
    # A bit hackish, but it works
    patchinfo.ramdisk    = 'jflte/AOSP/AOSP.def'
    patchinfo.patch      = [ autopatcher.auto_patch ]
    patchinfo.extract    = [ autopatcher.files_to_auto_patch ]

patchinfo.on_filename_set = on_filename_set

def matches(filename):
  regex = r"^I9505_-_Official_Google_Edition_.*Jamal2367.*\.zip$"
  return fileio.filename_matches(regex, filename) or \
    filename == 'I9505_-_Google_Edition_v6_by_Jamal2367.zip'

patchinfo.matches        = matches
