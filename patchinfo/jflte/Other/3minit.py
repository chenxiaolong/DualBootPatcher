from multiboot.autopatchers.standard import StandardPatcher
from multiboot.patchinfo import PatchInfo

patchinfo = PatchInfo()

patchinfo.matches        = r"^3Minit_Framework_.*\.zip$"
patchinfo.name           = '3Minit framework'
patchinfo.autopatchers   = [StandardPatcher]
patchinfo.has_boot_image = False
