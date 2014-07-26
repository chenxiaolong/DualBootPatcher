from multiboot.patchinfo import PatchInfo
import multiboot.autopatcher as autopatcher
import multiboot.fileio as fileio
from multiboot.autopatchers.cyanogenmod import DalvikCachePatcher

patchinfo = PatchInfo()

patchinfo.name           = 'CyanogenMod-based ROM'
patchinfo.ramdisk        = 'jflte/AOSP/AOSP.def'
patchinfo.patch          = [ autopatcher.auto_patch, DalvikCachePatcher.store_in_data ]
patchinfo.extract        = [ autopatcher.files_to_auto_patch, DalvikCachePatcher.files_for_store_in_data ]

def matches(filename):
  regex = r"^cm-[0-9\.]+-[0-9]+-.*.zip$"
  return fileio.filename_matches(regex, filename) \
    and 'noobdev' not in filename

patchinfo.matches        = matches
