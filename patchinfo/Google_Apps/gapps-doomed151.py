from multiboot.patchinfo import PatchInfo
import multiboot.autopatcher as autopatcher

patchinfo = PatchInfo()

patchinfo.matches        = r"^gapps-jb43-[0-9]+-dmd151\.zip$"
patchinfo.name           = "doomed151's Google Apps"
patchinfo.patch          = autopatcher.auto_patch
patchinfo.extract        = autopatcher.files_to_auto_patch
patchinfo.has_boot_image = False
