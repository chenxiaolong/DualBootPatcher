from multiboot.autopatchers.standard import StandardPatcher
from multiboot.patchinfo import PatchInfo

patchinfo = PatchInfo()

patchinfo.matches        = r"^nameless-.+\.zip$"
patchinfo.name           = 'NamelessROM'
patchinfo.ramdisk        = 'jflte/AOSP/AOSP.def'
patchinfo.autopatchers   = [StandardPatcher]
