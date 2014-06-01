from multiboot.patchinfo import PatchInfo
import multiboot.autopatcher as autopatcher

patchinfo = PatchInfo()

patchinfo.matches        = r"^SF-.*\.zip$"
patchinfo.name           = 'SkyFall'
patchinfo.ramdisk        = 'jflte/TouchWiz/TouchWiz.def'
patchinfo.patch          = autopatcher.auto_patch
patchinfo.extract        = autopatcher.files_to_auto_patch
