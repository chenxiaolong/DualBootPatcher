from multiboot.autopatchers.standard import StandardPatcher
from multiboot.patchinfo import PatchInfo

patchinfo = PatchInfo()

patchinfo.matches        = r"^Slim_AIO_gapps.*.zip$"
patchinfo.name           = 'SlimRoms Google Apps'
patchinfo.autopatchers   = [StandardPatcher]
patchinfo.has_boot_image = False
