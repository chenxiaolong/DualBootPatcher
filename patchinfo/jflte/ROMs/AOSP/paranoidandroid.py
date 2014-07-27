from multiboot.autopatchers.standard import StandardPatcher
from multiboot.patchinfo import PatchInfo
import multiboot.fileio as fileio

patchinfo = PatchInfo()

patchinfo.name           = 'ParanoidAndroid'
patchinfo.ramdisk        = 'jflte/AOSP/AOSP.def'
patchinfo.autopatchers   = [StandardPatcher]

def matches(filename):
  regex = r"^pa_[a-z0-9]+-.+-.+\.zip$"
  return fileio.filename_matches(regex, filename) \
    and 'gapps' not in filename

patchinfo.matches        = matches
