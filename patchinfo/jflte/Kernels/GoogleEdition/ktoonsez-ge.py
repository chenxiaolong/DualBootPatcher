from multiboot.autopatchers.standard import StandardPatcher
from multiboot.patchinfo import PatchInfo

patchinfo = PatchInfo()

patchinfo.matches        = r"^KT-SGS4-(JB4.3|KK4.4)-TWGE-.*\.zip$"
patchinfo.name           = 'Ktoonsez Google Edition kernel'
patchinfo.ramdisk        = 'jflte/GoogleEdition/GoogleEdition.def'
patchinfo.autopatchers   = [StandardPatcher]
