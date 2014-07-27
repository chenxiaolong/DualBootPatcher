from multiboot.autopatchers.standard import StandardPatcher
from multiboot.patchinfo import PatchInfo
import os

patchinfo = PatchInfo()

patchinfo.matches        = r"^GE.*\.zip$"
patchinfo.name           = 'GoldenEye'
patchinfo.ramdisk        = 'jflte/TouchWiz/TouchWiz.def'
patchinfo.autopatchers   = [StandardPatcher]
