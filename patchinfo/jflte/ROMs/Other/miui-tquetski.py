from multiboot.autopatchers.standard import StandardPatcher
from multiboot.patchinfo import PatchInfo

patchinfo = PatchInfo()

patchinfo.matches        = r"^MIUIv5-S4International-.+\.zip$"
patchinfo.name           = 'MIUI (Tquetski)'
patchinfo.ramdisk        = 'jflte/AOSP/AOSP.def'
patchinfo.autopatchers   = [StandardPatcher]
patchinfo.patched_init   = 'init-jb42'
