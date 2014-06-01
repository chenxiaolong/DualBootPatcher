from multiboot.patchinfo import PatchInfo

patchinfo = PatchInfo()

patchinfo.matches        = r"^UPDATE-SuperSU-v[0-9\.]+\.zip$"
patchinfo.name           = "Chainfire's SuperSU"
patchinfo.patch          = 'Other/supersu.dualboot.patch'
patchinfo.has_boot_image = False
