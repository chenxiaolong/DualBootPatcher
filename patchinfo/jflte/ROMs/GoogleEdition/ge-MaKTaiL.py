from multiboot.autopatchers.standard import StandardPatcher
from multiboot.patchinfo import PatchInfo

patchinfo = PatchInfo()

patchinfo.matches        = r"^i9505-ge-untouched-4.3-.*.zip$"
patchinfo.name           = "MaKTaiL's Google Edition"
patchinfo.ramdisk        = 'jflte/GoogleEdition/GoogleEdition.def'
patchinfo.autopatchers   = [StandardPatcher]
