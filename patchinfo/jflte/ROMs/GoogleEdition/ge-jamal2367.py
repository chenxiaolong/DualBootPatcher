from multiboot.autopatchers.standard import StandardPatcher
from multiboot.autopatchers.jflte import GoogleEditionPatcher
from multiboot.patchinfo import PatchInfo
import multiboot.fileio as fileio
import os

patchinfo = PatchInfo()

patchinfo.matches        = r"^I9505_-_Official_Google_Edition_.*Jamal2367.*\.zip$"
patchinfo.name           = "jamal2367's Google Edition"

def on_filename_set(patchinfo, filename):
  filename = os.path.split(filename)[1]
  if filename == 'I9505_-_Google_Edition_v6_by_Jamal2367.zip':
    # A bit hackish, but it works
    patchinfo.ramdisk    = 'jflte/AOSP/AOSP.def'
    patchinfo.autopatchers = [StandardPatcher]
  else:
    patchinfo.ramdisk    = 'jflte/GoogleEdition/GoogleEdition.def'
    patchinfo.autopatchers = [StandardPatcher, GoogleEditionPatcher]

patchinfo.on_filename_set = on_filename_set

def matches(filename):
  regex = r"^I9505_-_Official_Google_Edition_.*Jamal2367.*\.zip$"
  return fileio.filename_matches(regex, filename) or \
    filename == 'I9505_-_Google_Edition_v6_by_Jamal2367.zip'

patchinfo.matches        = matches
