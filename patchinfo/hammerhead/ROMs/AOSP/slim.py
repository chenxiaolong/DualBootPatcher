from multiboot.autopatchers.standard import StandardPatcher
from multiboot.patchinfo import PatchInfo

patchinfo = PatchInfo()

patchinfo.matches        = r"^Slim-.*.zip$"
patchinfo.name           = 'SlimRoms'
patchinfo.ramdisk        = 'hammerhead/AOSP/AOSP.def'
patchinfo.autopatchers   = [StandardPatcher]
