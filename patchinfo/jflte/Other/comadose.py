from multiboot.patchinfo import PatchInfo

patchinfo = PatchInfo()

patchinfo.matches        = r"^ComaDose_V[0-9\.]+_Cossbreeder_[0-9\.]+\.zip"
patchinfo.name           = 'ComaDose'
patchinfo.autopatchers   = ['jflte/Other/comadose.dualboot.patch']
patchinfo.has_boot_image = False
