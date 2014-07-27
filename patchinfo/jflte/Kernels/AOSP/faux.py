from multiboot.autopatchers.standard import StandardPatcher
from multiboot.patchinfo import PatchInfo
import os

patchinfo = PatchInfo()

patchinfo.matches        = r"^jflte[a-z]+-aosp-faux123-.*\.zip$"
patchinfo.name           = 'Faux kernel'
patchinfo.ramdisk        = 'jflte/AOSP/AOSP.def'
patchinfo.autopatchers   = [StandardPatcher]
