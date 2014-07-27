from multiboot.autopatchers.standard import StandardPatcher
from multiboot.patchinfo import PatchInfo

patchinfo = PatchInfo()

patchinfo.matches        = r"^miuiandroid_gapps.*\.zip$"
patchinfo.name           = 'MIUI Google Apps'
patchinfo.autopatchers   = [StandardPatcher]
patchinfo.has_boot_image = False
