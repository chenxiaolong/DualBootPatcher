from multiboot.patchinfo import PatchInfo
import multiboot.autopatcher as autopatcher
import os

patchinfo = PatchInfo()

patchinfo.matches        = r"^jflte[a-z]+-aosp-faux123-.*\.zip$"
patchinfo.name           = 'Faux kernel'
patchinfo.ramdisk        = 'jflte/AOSP/AOSP.def'
patchinfo.patch          = autopatcher.auto_patch
patchinfo.extract        = autopatcher.files_to_auto_patch
