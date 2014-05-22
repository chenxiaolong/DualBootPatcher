from multiboot.fileinfo import FileInfo
import multiboot.autopatcher as autopatcher

file_info = FileInfo()

filename_regex           = r"^.*_AdamKernel.V[0-9\.]+\.CWM\.zip$"
file_info.name           = 'Adam kernel'
file_info.ramdisk        = 'jflte/TouchWiz/TouchWiz.def'
file_info.patch          = autopatcher.auto_patch
file_info.extract        = autopatcher.files_to_auto_patch
file_info.bootimg        = "wanam/boot.img"
