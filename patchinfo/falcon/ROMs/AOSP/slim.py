from multiboot.autopatchers.standard import StandardPatcher
from multiboot.patchinfo import PatchInfo

patchinfo = PatchInfo()

patchinfo.matches        = r"^Slim-.*.zip$"
patchinfo.name           = 'SlimRoms'
patchinfo.ramdisk        = 'falcon/AOSP/AOSP.def'
patchinfo.autopatchers   = [StandardPatcher]
