from multiboot.patchinfo import PatchInfo
import multiboot.autopatcher as autopatcher
import multiboot.fileio as fileio

patchinfo = PatchInfo()

patchinfo.matches        = r"^[a-zA-Z0-9]+BlackBox[0-9]+SGS4\.zip$"
patchinfo.name           = 'BlackBox TouchWiz'
patchinfo.ramdisk        = 'jflte/TouchWiz/TouchWiz.def'
patchinfo.patch          = autopatcher.auto_patch
patchinfo.extract        = autopatcher.files_to_auto_patch
