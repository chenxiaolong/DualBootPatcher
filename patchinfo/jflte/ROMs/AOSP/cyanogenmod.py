from multiboot.patchinfo import PatchInfo
import multiboot.autopatcher as autopatcher
from multiboot.autopatchers.cyanogenmod import DalvikCachePatcher

patchinfo = PatchInfo()

patchinfo.matches        = r"^cm-[0-9\.]+(-[0-9]+-NIGHTLY|-RC[0-9]+)?-[a-z0-9]+\.zip$"
patchinfo.name           = 'Official CyanogenMod'
patchinfo.ramdisk        = 'jflte/AOSP/AOSP.def'
patchinfo.patch          = [ autopatcher.auto_patch, DalvikCachePatcher.store_in_data ]
patchinfo.extract        = [ autopatcher.files_to_auto_patch, DalvikCachePatcher.files_for_store_in_data ]
