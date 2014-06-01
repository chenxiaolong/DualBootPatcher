from multiboot.patchinfo import PatchInfo
import multiboot.autopatcher as autopatcher

patchinfo = PatchInfo()

patchinfo.matches        = r"^Cataclysm.*\.zip$"
patchinfo.name           = 'Cataclysm'
patchinfo.ramdisk        = 'hammerhead/AOSP/AOSP.def'
patchinfo.patch          = autopatcher.auto_patch
patchinfo.extract        = autopatcher.files_to_auto_patch
patchinfo.patched_init   = 'init-kk44'
