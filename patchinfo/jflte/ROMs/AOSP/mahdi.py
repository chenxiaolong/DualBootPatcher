# Thanks to visanciprian for this file!
# http://forum.xda-developers.com/showpost.php?p=49218687&postcount=1733

from multiboot.patchinfo import PatchInfo
import multiboot.autopatcher as autopatcher

patchinfo = PatchInfo()

patchinfo.matches        = r"^mahdi-.*jflte.*\.zip$"
patchinfo.name           = 'Mahdi'
patchinfo.ramdisk        = 'jflte/AOSP/AOSP.def'
patchinfo.patch          = autopatcher.auto_patch
patchinfo.extract        = autopatcher.files_to_auto_patch
