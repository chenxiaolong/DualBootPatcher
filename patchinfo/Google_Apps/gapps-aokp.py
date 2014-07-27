from multiboot.autopatchers.standard import StandardPatcher
from multiboot.patchinfo import PatchInfo

patchinfo = PatchInfo()

patchinfo.matches        = r"^aokp-(full_|mini_)?gapps.+\.zip$"
patchinfo.name           = 'AOKP Google Apps'
patchinfo.autopatchers   = [StandardPatcher]
patchinfo.has_boot_image = False
