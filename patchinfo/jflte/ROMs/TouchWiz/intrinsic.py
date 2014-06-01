from multiboot.patchinfo import PatchInfo
import os

patchinfo = PatchInfo()

patchinfo.name           = 'iNTriNsiC'
patchinfo.ramdisk        = 'jflte/TouchWiz/TouchWiz.def'
patchinfo.patch          = 'jflte/ROMs/TouchWiz/intrinsic-20130806.dualboot.patch'

def matches(filename):
  filename = os.path.split(filename)[1]
  return filename == "iNTriNsiC 8-6-13.zip"

patchinfo.matches        = matches
