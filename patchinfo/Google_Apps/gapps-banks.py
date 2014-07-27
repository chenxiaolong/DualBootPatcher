from multiboot.autopatchers.standard import StandardPatcher
from multiboot.patchinfo import PatchInfo

patchinfo = PatchInfo()

patchinfo.matches        = r"^[0-9]+-[0-9]+_GApps_(Standard|Minimal|Core)_[0-9\.]+_signed\.zip$"
patchinfo.name           = "BaNks's Google Apps"
patchinfo.autopatchers   = [StandardPatcher]
patchinfo.has_boot_image = False
