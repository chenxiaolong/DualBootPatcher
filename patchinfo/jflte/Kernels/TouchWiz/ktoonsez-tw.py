from multiboot.autopatchers.standard import StandardPatcher
from multiboot.patchinfo import PatchInfo

patchinfo = PatchInfo()

patchinfo.matches        = r"^KT-SGS4-(JB|KK)4.[2-4]-TW-.*\.zip$"
patchinfo.name           = 'Ktoonsez TouchWiz kernel'
patchinfo.ramdisk        = 'jflte/TouchWiz/TouchWiz.def'
patchinfo.autopatchers   = [StandardPatcher]
