from multiboot.autopatchers.standard import StandardPatcher
from multiboot.patchinfo import PatchInfo

patchinfo = PatchInfo()

patchinfo.matches        = r"^Helly?(bean|kat)-.*\.zip$"
patchinfo.name           = 'HellyBean/HellyKat'
patchinfo.ramdisk        = 'jflte/AOSP/AOSP.def'
patchinfo.autopatchers   = [StandardPatcher]
patchinfo.device_check   = False
