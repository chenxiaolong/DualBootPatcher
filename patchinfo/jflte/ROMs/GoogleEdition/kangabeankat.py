from multiboot.patchinfo import PatchInfo
import multiboot.autopatcher as autopatcher
from multiboot.autopatchers.jflte import GoogleEditionPatcher
import os
import re

patchinfo = PatchInfo()

patchinfo.matches        = r"^K[BK]-.*\.zip$"
patchinfo.name           = 'Kangabean/Kangakat'
patchinfo.ramdisk        = 'jflte/GoogleEdition/GoogleEdition.def'

def on_filename_set(patchinfo, filename):
  filename = os.path.split(filename)[1]
  if filename.startswith("KK"):
    patchinfo.patch      = [ autopatcher.auto_patch, GoogleEditionPatcher.qcom_audio_fix ]
    patchinfo.extract    = [ autopatcher.files_to_auto_patch, GoogleEditionPatcher.files_for_qcom_audio_fix ]
  elif filename.startswith("KB"):
    patchinfo.patch      = [ autopatcher.auto_patch ]
    patchinfo.extract    = [ autopatcher.files_to_auto_patch ]

patchinfo.on_filename_set = on_filename_set
