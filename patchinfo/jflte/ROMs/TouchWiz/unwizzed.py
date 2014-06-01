from multiboot.patchinfo import PatchInfo

patchinfo = PatchInfo()

patchinfo.matches        = r"^Evil_UnWizzed_v[0-9]+\.zip$"
patchinfo.name           = 'Evil UnWizzed'
patchinfo.ramdisk        = 'jflte/TouchWiz/TouchWiz.def'
patchinfo.patch          = 'jflte/ROMs/TouchWiz/unwizzed.dualboot.patch'
