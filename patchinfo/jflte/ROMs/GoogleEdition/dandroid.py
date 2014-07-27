from multiboot.autopatchers.standard import StandardPatcher
from multiboot.autopatchers.jflte import GoogleEditionPatcher
from multiboot.patchinfo import PatchInfo

patchinfo = PatchInfo()

patchinfo.matches        = r"^(danvdh-GE|Dandroid).*\.zip$"
patchinfo.name           = 'Dandroid'
patchinfo.ramdisk        = 'jflte/GoogleEdition/GoogleEdition.def'
patchinfo.autopatchers   = [StandardPatcher, GoogleEditionPatcher]
