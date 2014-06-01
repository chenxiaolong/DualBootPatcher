from multiboot.patchinfo import PatchInfo
import multiboot.autopatcher as autopatcher

patchinfo = PatchInfo()

patchinfo.matches        = r"^[0-9]+-[0-9]+_GApps_(Standard|Minimal|Core)_[0-9\.]+_signed\.zip$"
patchinfo.name           = "BaNks's Google Apps"
patchinfo.patch          = autopatcher.auto_patch
patchinfo.extract        = autopatcher.files_to_auto_patch
patchinfo.has_boot_image = False
