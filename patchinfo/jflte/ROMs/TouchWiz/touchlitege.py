from multiboot.patchinfo import PatchInfo
import multiboot.autopatcher as autopatcher

patchinfo = PatchInfo()

patchinfo.matches        = r"^touchliteGE.*\.zip$"
patchinfo.name           = 'TouchliteGE TouchWiz'
patchinfo.ramdisk        = 'jflte/TouchWiz/TouchWiz.def'
patchinfo.patch          = autopatcher.auto_patch
patchinfo.extract        = autopatcher.files_to_auto_patch
