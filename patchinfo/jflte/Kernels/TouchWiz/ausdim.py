from multiboot.autopatchers.standard import StandardPatcher
from multiboot.patchinfo import PatchInfo

patchinfo = PatchInfo()

patchinfo.matches        = r"^v[0-9\.]+-(TW|Google)-[Ee]dition-ausdim-Kernel-.*\.zip$"
patchinfo.name           = 'Ausdim kernel'
patchinfo.ramdisk        = 'jflte/TouchWiz/TouchWiz.def'
patchinfo.autopatchers   = [StandardPatcher]
