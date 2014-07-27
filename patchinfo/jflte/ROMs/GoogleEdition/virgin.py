from multiboot.autopatchers.standard import StandardPatcher
from multiboot.autopatchers.jflte import GoogleEditionPatcher
from multiboot.patchinfo import PatchInfo

patchinfo = PatchInfo()

patchinfo.matches        = r"^VirginROM.*\.zip$"
patchinfo.name           = 'VirginROM'
patchinfo.ramdisk        = 'jflte/GoogleEdition/GoogleEdition.def'
patchinfo.autopatchers   = [StandardPatcher, GoogleEditionPatcher]
