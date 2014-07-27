from multiboot.autopatchers.standard import StandardPatcher
from multiboot.patchinfo import PatchInfo

patchinfo = PatchInfo()

patchinfo.matches        = r"^Kernel-Alucard.*AOSP.*\.zip$"
patchinfo.name           = 'Alucard AOSP kernel'
patchinfo.ramdisk        = 'jflte/AOSP/AOSP.def'
patchinfo.autopatchers   = [StandardPatcher]
patchinfo.patched_init   = 'init-kk44'
