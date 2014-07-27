from multiboot.autopatchers.standard import StandardPatcher
from multiboot.patchinfo import PatchInfo

patchinfo = PatchInfo()

patchinfo.matches        = r"^gapps-kk-[0-9]{8}\.zip$"
patchinfo.name           = 'Gummy Google Apps'
patchinfo.autopatchers   = [StandardPatcher]
patchinfo.has_boot_image = False
