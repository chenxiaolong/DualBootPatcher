from multiboot.patchinfo import PatchInfo
import multiboot.autopatcher as autopatcher

patchinfo = PatchInfo()

patchinfo.matches        = r"^cm-[0-9\.]+(-[0-9]+-NIGHTLY)?-[a-z0-9]+\.zip$"
patchinfo.name           = 'Official CyanogenMod'
patchinfo.ramdisk        = 'hammerhead/AOSP/AOSP.def'
patchinfo.patch          = autopatcher.auto_patch
patchinfo.extract        = autopatcher.files_to_auto_patch
