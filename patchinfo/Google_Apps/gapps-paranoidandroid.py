from multiboot.autopatchers.standard import StandardPatcher
from multiboot.patchinfo import PatchInfo

patchinfo = PatchInfo()

patchinfo.matches        = r"^pa_gapps-.*-[0-9\.]+-[0-9]+-signed\.zip$"
patchinfo.name           = 'Paranoid Android Google Apps'
patchinfo.autopatchers   = [StandardPatcher]
patchinfo.has_boot_image = False
