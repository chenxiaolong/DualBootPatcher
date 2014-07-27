from multiboot.autopatchers.standard import StandardPatcher
from multiboot.patchinfo import PatchInfo

patchinfo = PatchInfo()

patchinfo.matches        = r"^miuiandroid_(multi_)?jflte.*\.zip$"
patchinfo.name           = 'MIUI'
patchinfo.ramdisk        = 'jflte/AOSP/AOSP.def'
patchinfo.autopatchers   = [StandardPatcher]
patchinfo.patched_init   = 'init-jb42'
