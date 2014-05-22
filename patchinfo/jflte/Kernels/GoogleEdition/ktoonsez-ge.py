from multiboot.fileinfo import FileInfo
import multiboot.autopatcher as autopatcher

file_info = FileInfo()

filename_regex           = r"^KT-SGS4-(JB4.3|KK4.4)-TWGE-.*\.zip$"
file_info.name           = 'Ktoonsez Google Edition kernel'
file_info.ramdisk        = 'jflte/GoogleEdition/GoogleEdition.def'
file_info.patch          = autopatcher.auto_patch
file_info.extract        = autopatcher.files_to_auto_patch
