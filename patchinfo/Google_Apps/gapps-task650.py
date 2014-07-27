from multiboot.autopatchers.standard import StandardPatcher
from multiboot.patchinfo import PatchInfo

patchinfo = PatchInfo()

patchinfo.matches        = r"^gapps-jb\([0-9\.]+\)-[0-9\.]+\.zip$"
patchinfo.name           = "Task650's Google Apps"
patchinfo.autopatchers   = [StandardPatcher]
patchinfo.has_boot_image = False
