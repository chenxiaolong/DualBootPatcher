from multiboot.patchinfo import PatchInfo
import os

patchinfo = PatchInfo()

patchinfo.name           = 'Xposed Framework Disabler'
patchinfo.autopatchers   = ['Other/xposed.dualboot.patch']
patchinfo.has_boot_image = False

def matches(filename):
  filename = os.path.split(filename)[1]
  return filename == 'Xposed-Disabler-Recovery.zip'

patchinfo.matches        = matches
