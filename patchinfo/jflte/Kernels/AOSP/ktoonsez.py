from multiboot.patchinfo import PatchInfo
import multiboot.autopatcher as autopatcher

patchinfo = PatchInfo()

patchinfo.matches        = r"^KT-SGS4-[^-]+-AOSP-.*\.zip$"
patchinfo.name           = 'Ktoonsez AOSP kernel'
patchinfo.ramdisk        = 'jflte/AOSP/ktoonsez.def'
patchinfo.patch          = autopatcher.auto_patch
patchinfo.extract        = autopatcher.files_to_auto_patch
