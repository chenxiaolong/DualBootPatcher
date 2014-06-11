from multiboot.patchinfo import PatchInfo
import multiboot.autopatcher as autopatcher
from multiboot.autopatchers.jflte import GoogleEditionPatcher
import os
import re

patchinfo = PatchInfo()

patchinfo.name           = 'Echoe Google Edition'
patchinfo.ramdisk        = 'jflte/GoogleEdition/GoogleEdition.def'
patchinfo.patch          = [ autopatcher.auto_patch, GoogleEditionPatcher.qcom_audio_fix ]
patchinfo.extract        = [ autopatcher.files_to_auto_patch, GoogleEditionPatcher.files_for_qcom_audio_fix ]

def matches(filename):
  filename = os.path.split(filename)[1]
  return re.search(r'^Echoe_KitKat.+\.zip$', filename) is not None or \
    re.search(r'^EchoeGE.+\.zip$', filename) is not None

patchinfo.matches        = matches
