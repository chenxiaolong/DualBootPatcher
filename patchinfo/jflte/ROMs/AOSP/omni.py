from multiboot.autopatchers.standard import StandardPatcher
from multiboot.patchinfo import PatchInfo

patchinfo = PatchInfo()

patchinfo.matches        = r"^omni-[0-9\.]+-[0-9]+.*\.zip$"
patchinfo.name           = 'Omni'
patchinfo.ramdisk        = 'jflte/AOSP/AOSP.def'
patchinfo.autopatchers   = [StandardPatcher]
