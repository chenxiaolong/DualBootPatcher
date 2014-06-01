from multiboot.patchinfo import PatchInfo
import multiboot.autopatcher as autopatcher

patchinfo = PatchInfo()

patchinfo.matches        = r"^(Liquid|LS)-(JB|KK|Kitkat)-v[0-9\.]+-.*-jflte.*\.zip$"
patchinfo.name           = 'LiquidSmooth'
patchinfo.ramdisk        = 'jflte/AOSP/AOSP.def'
patchinfo.patch          = autopatcher.auto_patch
patchinfo.extract        = autopatcher.files_to_auto_patch
