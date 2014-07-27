from multiboot.autopatchers.standard import StandardPatcher
from multiboot.patchinfo import PatchInfo

patchinfo = PatchInfo()

patchinfo.matches        = r"^pac_.*-Black-Power-Edition(_NG)?[-_][0-9]+\.zip$"
patchinfo.name           = "Metaiiica's PAC-Man"
patchinfo.ramdisk        = 'jflte/AOSP/AOSP.def'
patchinfo.autopatchers   = [StandardPatcher]
