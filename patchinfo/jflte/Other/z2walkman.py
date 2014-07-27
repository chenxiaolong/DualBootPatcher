from multiboot.autopatchers.standard import StandardPatcher
from multiboot.patchinfo import PatchInfo
import os

patchinfo = PatchInfo()

patchinfo.name           = 'Xperia Z2 WALKMAN'
patchinfo.autopatchers   = [StandardPatcher]
patchinfo.has_boot_image = False

def matches(filename):
  filename = os.path.split(filename)[1]
  return 'Xperia_Z2_WALKMAN' in filename

patchinfo.matches        = matches
