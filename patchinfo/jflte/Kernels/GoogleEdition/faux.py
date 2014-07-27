from multiboot.autopatchers.standard import StandardPatcher
from multiboot.patchinfo import PatchInfo

patchinfo = PatchInfo()

patchinfo.matches        = r"^j(ged|f)lte([a-z]+)?-faux123-GE-.*\.zip$"
patchinfo.name           = 'Faux Google Edition kernel'
patchinfo.ramdisk        = 'jflte/GoogleEdition/GoogleEdition.def'
patchinfo.autopatchers   = [StandardPatcher]
