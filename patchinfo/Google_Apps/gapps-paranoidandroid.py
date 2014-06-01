from multiboot.patchinfo import PatchInfo
import multiboot.autopatcher as autopatcher

patchinfo = PatchInfo()

patchinfo.matches        = r"^pa_gapps-.*-[0-9\.]+-[0-9]+-signed\.zip$"
patchinfo.name           = 'Paranoid Android Google Apps'
patchinfo.patch          = autopatcher.auto_patch
patchinfo.extract        = autopatcher.files_to_auto_patch
patchinfo.has_boot_image = False
