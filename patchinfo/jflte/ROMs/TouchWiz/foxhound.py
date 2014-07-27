from multiboot.autopatchers.standard import StandardPatcher
from multiboot.patchinfo import PatchInfo

patchinfo = PatchInfo()

patchinfo.matches        = r"^FoxHound_.*\.zip$"
patchinfo.name           = 'FoxHound'
patchinfo.ramdisk        = 'jflte/TouchWiz/TouchWiz.def'
patchinfo.autopatchers   = [StandardPatcher,
                            'jflte/ROMs/TouchWiz/foxhound.dualboot.patch']
