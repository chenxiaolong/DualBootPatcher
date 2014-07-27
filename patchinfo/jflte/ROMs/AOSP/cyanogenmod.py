from multiboot.autopatchers.standard import StandardPatcher
from multiboot.autopatchers.cyanogenmod import DalvikCachePatcher
from multiboot.patchinfo import PatchInfo

patchinfo = PatchInfo()

patchinfo.matches        = r"^cm-[0-9\.]+(-[0-9]+-NIGHTLY|-RC[0-9]+)?-[a-z0-9]+\.zip$"
patchinfo.name           = 'Official CyanogenMod'
patchinfo.ramdisk        = 'jflte/AOSP/AOSP.def'
patchinfo.autopatchers   = [StandardPatcher, DalvikCachePatcher]
