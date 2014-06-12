from multiboot.patchinfo import PatchInfo
import multiboot.autopatcher as autopatcher

patchinfo = PatchInfo()

patchinfo.matches        = r'^AEL_GE.+\.zip$'
patchinfo.name           = 'Echoe AEL Google Edition kernel'
patchinfo.ramdisk        = 'jflte/GoogleEdition/GoogleEdition.def'
patchinfo.patch          = autopatcher.auto_patch
patchinfo.extract        = autopatcher.files_to_auto_patch
