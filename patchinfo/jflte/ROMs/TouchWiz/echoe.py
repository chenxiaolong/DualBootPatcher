from multiboot.autopatchers.standard import StandardPatcher
from multiboot.patchinfo import PatchInfo
import multiboot.fileio as fileio
import os

patchinfo = PatchInfo()

patchinfo.name           = 'Echoe TouchWiz'
patchinfo.ramdisk        = 'jflte/TouchWiz/TouchWiz.def'
patchinfo.autopatchers   = [StandardPatcher]

def matches(filename):
  regexes = [ r"^Echoe[ _]?(Rom|SLIM).*\.zip$",
              r"^S4_Echoe.*\.zip",
              r'^Echoe_v1[478].*\.zip',
              r'^S4_Echoe_v2[0].*\.zip' ]
  for regex in regexes:
    if fileio.filename_matches(regex, filename):
      return True
  return False

patchinfo.matches        = matches
