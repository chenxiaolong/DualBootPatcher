from multiboot.autopatchers.standard import StandardPatcher
from multiboot.patchinfo import PatchInfo

patchinfo = PatchInfo()

patchinfo.matches        = r'^Kernel-Alucard.*TW.*\.zip$'
patchinfo.name           = 'Alucard TouchWiz kernel'
patchinfo.ramdisk        = 'jflte/TouchWiz/TouchWiz.def'
patchinfo.autopatchers   = [StandardPatcher]
