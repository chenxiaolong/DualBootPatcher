from multiboot.patchinfo import PatchInfo
import multiboot.autopatcher as autopatcher
import os

patchinfo = PatchInfo()

patchinfo.name           = 'Xperia Z2 WALKMAN'
patchinfo.patch          = autopatcher.auto_patch
patchinfo.extract        = autopatcher.files_to_auto_patch
patchinfo.has_boot_image = False

def matches(filename):
  filename = os.path.split(filename)[1]
  return 'Xperia_Z2_WALKMAN' in filename

patchinfo.matches        = matches
