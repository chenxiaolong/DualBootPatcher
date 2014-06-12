from multiboot.patchinfo import PatchInfo
import multiboot.autopatcher as autopatcher

patchinfo = PatchInfo()

patchinfo.matches        = r"^aokp-(full_|mini_)?gapps.+\.zip$"
patchinfo.name           = 'AOKP Google Apps'
patchinfo.patch          = autopatcher.auto_patch
patchinfo.extract        = autopatcher.files_to_auto_patch
patchinfo.has_boot_image = False
