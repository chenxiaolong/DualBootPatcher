from multiboot.patchinfo import PatchInfo
import multiboot.autopatcher as autopatcher
import os

patchinfo = PatchInfo()

patchinfo.name           = "geiti94's HTC Sense 5 port"
patchinfo.ramdisk        = 'jflte/AOSP/AOSP.def'
patchinfo.patch          = autopatcher.auto_patch
patchinfo.extract        = autopatcher.files_to_auto_patch

def matches(filename):
  filename = os.path.split(filename)[1]
  return filename.lower() == "sense5port.zip" or \
         filename         == "4.3 official port.zip" or \
         filename         == "ALPHA V1.5.zip"

patchinfo.matches        = matches
