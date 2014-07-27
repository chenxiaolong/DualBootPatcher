from multiboot.autopatchers.standard import StandardPatcher
from multiboot.patchinfo import PatchInfo

patchinfo = PatchInfo()

patchinfo.matches        = r"^Cataclysm.*\.zip$"
patchinfo.name           = 'Cataclysm'
patchinfo.ramdisk        = 'hammerhead/AOSP/AOSP.def'
patchinfo.autopatchers   = [StandardPatcher]
patchinfo.patched_init   = 'init-kk44'
