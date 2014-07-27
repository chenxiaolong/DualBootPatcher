from multiboot.autopatchers.standard import StandardPatcher
from multiboot.autopatchers.jflte import GoogleEditionPatcher
from multiboot.patchinfo import PatchInfo

patchinfo = PatchInfo()

patchinfo.matches        = r"^[a-zA-Z0-9]+BlackBoxGE[0-9]+\.zip$"
patchinfo.name           = 'BlackBox Google Edition'
patchinfo.ramdisk        = 'jflte/GoogleEdition/GoogleEdition.def'
patchinfo.autopatchers   = [StandardPatcher, GoogleEditionPatcher]
