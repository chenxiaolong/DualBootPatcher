from multiboot.autopatchers.standard import StandardPatcher
from multiboot.patchinfo import PatchInfo

patchinfo = PatchInfo()

patchinfo.matches        = r"^JellyBeer-.*\.zip$"
patchinfo.name           = 'JellyBeer'
patchinfo.ramdisk        = 'jflte/AOSP/AOSP.def'
patchinfo.patched_init   = 'init-jb42'
patchinfo.autopatchers   = [StandardPatcher]
