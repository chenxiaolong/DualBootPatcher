from multiboot.autopatchers.standard import StandardPatcher
from multiboot.patchinfo import PatchInfo

patchinfo = PatchInfo()

patchinfo.matches        = r"^MODIFIED_STOCK_.*\.zip$"
patchinfo.name           = "Omnifarious's modified stock TouchWiz"
patchinfo.ramdisk        = 'jflte/TouchWiz/TouchWiz.def'
patchinfo.autopatchers   = [StandardPatcher]
