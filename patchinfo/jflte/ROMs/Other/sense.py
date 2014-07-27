from multiboot.autopatchers.standard import StandardPatcher
from multiboot.patchinfo import PatchInfo
import os

patchinfo = PatchInfo()

patchinfo.name           = "geiti94's HTC Sense 5 port"
patchinfo.ramdisk        = 'jflte/AOSP/AOSP.def'
patchinfo.autopatchers   = [StandardPatcher]

def matches(filename):
  filename = os.path.split(filename)[1]
  return filename.lower() == "sense5port.zip" or \
         filename         == "4.3 official port.zip" or \
         filename         == "ALPHA V1.5.zip"

patchinfo.matches        = matches
