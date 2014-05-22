from multiboot.fileinfo import FileInfo
import multiboot.autopatcher as autopatcher
from multiboot.autopatchers.jflte import GoogleEditionPatcher

file_info = FileInfo()

filename_regex           = r"^[a-zA-Z0-9]+BlackBoxGE[0-9]+\.zip$"
file_info.name           = 'BlackBox Google Edition'
file_info.ramdisk        = 'jflte/GoogleEdition/GoogleEdition.def'
file_info.patch          = [ autopatcher.auto_patch, GoogleEditionPatcher.qcom_audio_fix ]
file_info.extract        = [ autopatcher.files_to_auto_patch, GoogleEditionPatcher.files_for_qcom_audio_fix ]
file_info.bootimg        = 'dsa/Kernels/Stock/boot.img'
