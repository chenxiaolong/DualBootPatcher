from multiboot.autopatchers.standard import StandardPatcher
from multiboot.patchinfo import PatchInfo

patchinfo = PatchInfo()

patchinfo.matches        = r"^gapps-jb43-[0-9]+-dmd151\.zip$"
patchinfo.name           = "doomed151's Google Apps"
patchinfo.autopatchers   = [StandardPatcher]
patchinfo.has_boot_image = False
