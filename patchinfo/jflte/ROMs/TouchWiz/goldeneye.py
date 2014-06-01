from multiboot.patchinfo import PatchInfo
import multiboot.autopatcher as autopatcher
import os

patchinfo = PatchInfo()

patchinfo.matches        = r"^GE.*\.zip$"
patchinfo.name           = 'GoldenEye'
patchinfo.ramdisk        = 'jflte/TouchWiz/TouchWiz.def'
patchinfo.patch          = autopatcher.auto_patch
patchinfo.extract        = autopatcher.files_to_auto_patch
