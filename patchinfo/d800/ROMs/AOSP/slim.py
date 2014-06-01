from multiboot.patchinfo import PatchInfo
import multiboot.autopatcher as autopatcher

patchinfo = PatchInfo()

patchinfo.matches        = r"^Slim-.*.zip$"
patchinfo.name           = 'SlimRoms'
patchinfo.ramdisk        = 'd800/AOSP/AOSP.def'
patchinfo.patch          = autopatcher.auto_patch
patchinfo.extract        = autopatcher.files_to_auto_patch
