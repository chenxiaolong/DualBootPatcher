from multiboot.autopatchers.standard import StandardPatcher
from multiboot.autopatchers.jflte import GoogleEditionPatcher
from multiboot.patchinfo import PatchInfo
import os
import re

patchinfo = PatchInfo()

patchinfo.name           = 'Echoe Google Edition'
patchinfo.ramdisk        = 'jflte/GoogleEdition/GoogleEdition.def'
patchinfo.autopatchers   = [StandardPatcher, GoogleEditionPatcher]

def matches(filename):
  filename = os.path.split(filename)[1]
  return re.search(r'^Echoe_KitKat.+\.zip$', filename) is not None or \
    re.search(r'^EchoeGE.+\.zip$', filename) is not None

patchinfo.matches        = matches
