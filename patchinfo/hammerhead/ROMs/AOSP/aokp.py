from multiboot.autopatchers.standard import StandardPatcher
from multiboot.patchinfo import PatchInfo

patchinfo = PatchInfo()

patchinfo.matches        = r"^aokp_hammerhead_.*\.zip$"
patchinfo.name           = 'AOKP'
patchinfo.ramdisk        = 'hammerhead/AOSP/AOSP.def'
patchinfo.autopatchers   = [StandardPatcher]
