from multiboot.autopatchers.standard import StandardPatcher
from multiboot.patchinfo import PatchInfo

patchinfo = PatchInfo()

patchinfo.matches        = r"^cm-[0-9\.]+(-[0-9]+-NIGHTLY)?-[a-z0-9]+\.zip$"
patchinfo.name           = 'Official CyanogenMod'
patchinfo.ramdisk        = 'hammerhead/AOSP/AOSP.def'
patchinfo.autopatchers   = [StandardPatcher]
