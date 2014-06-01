from multiboot.patchinfo import PatchInfo
import multiboot.autopatcher as autopatcher

patchinfo = PatchInfo()

patchinfo.matches        = r"^JellyBeer-.*\.zip$"
patchinfo.name           = 'JellyBeer'
patchinfo.ramdisk        = 'jflte/AOSP/AOSP.def'
patchinfo.patched_init   = 'init-jb42'
patchinfo.patch          = autopatcher.auto_patch
patchinfo.extract        = autopatcher.files_to_auto_patch
