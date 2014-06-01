from multiboot.patchinfo import PatchInfo
import multiboot.autopatcher as autopatcher
from multiboot.autopatchers.jflte import GoogleEditionPatcher

patchinfo = PatchInfo()

patchinfo.matches        = r"^[a-zA-Z0-9]+BlackBoxGE[0-9]+\.zip$"
patchinfo.name           = 'BlackBox Google Edition'
patchinfo.ramdisk        = 'jflte/GoogleEdition/GoogleEdition.def'
patchinfo.patch          = [ autopatcher.auto_patch, GoogleEditionPatcher.qcom_audio_fix ]
patchinfo.extract        = [ autopatcher.files_to_auto_patch, GoogleEditionPatcher.files_for_qcom_audio_fix ]
