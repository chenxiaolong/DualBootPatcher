from multiboot.autopatchers.standard import StandardPatcher
from multiboot.patchinfo import PatchInfo

patchinfo = PatchInfo()

patchinfo.matches        = r"^(Liquid|LS)-(JB|KK|Kitkat)-v[0-9\.]+-.*-jflte.*\.zip$"
patchinfo.name           = 'LiquidSmooth'
patchinfo.ramdisk        = 'jflte/AOSP/AOSP.def'
patchinfo.autopatchers   = [StandardPatcher]
