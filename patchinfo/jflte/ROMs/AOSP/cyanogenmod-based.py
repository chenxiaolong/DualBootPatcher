from multiboot.autopatchers.standard import StandardPatcher
from multiboot.autopatchers.cyanogenmod import DalvikCachePatcher
from multiboot.patchinfo import PatchInfo
import multiboot.fileio as fileio

patchinfo = PatchInfo()

patchinfo.name           = 'CyanogenMod-based ROM'
patchinfo.ramdisk        = 'jflte/AOSP/AOSP.def'
patchinfo.autopatchers   = [StandardPatcher, DalvikCachePatcher]

def matches(filename):
  regex = r"^cm-[0-9\.]+-[0-9]+-.*.zip$"
  return fileio.filename_matches(regex, filename) \
    and 'noobdev' not in filename

patchinfo.matches        = matches
