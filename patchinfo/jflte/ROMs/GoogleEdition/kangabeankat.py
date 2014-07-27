from multiboot.autopatchers.standard import StandardPatcher
from multiboot.autopatchers.jflte import GoogleEditionPatcher
from multiboot.patchinfo import PatchInfo
import os

patchinfo = PatchInfo()

patchinfo.matches        = r"^K[BK]-.*\.zip$"
patchinfo.name           = 'Kangabean/Kangakat'
patchinfo.ramdisk        = 'jflte/GoogleEdition/GoogleEdition.def'

def on_filename_set(patchinfo, filename):
  filename = os.path.split(filename)[1]
  if filename.startswith("KK"):
    patchinfo.autopatchers= [StandardPatcher, GoogleEditionPatcher]
  elif filename.startswith("KB"):
    patchinfo.autopatchers= [StandardPatcher]

patchinfo.on_filename_set = on_filename_set
