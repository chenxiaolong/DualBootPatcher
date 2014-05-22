from multiboot.fileinfo import FileInfo
import multiboot.autopatcher as autopatcher

file_info = FileInfo()

filename_regex           = r"^KT-SGS4-(JB|KK)4.[2-4]-TW-.*\.zip$"
file_info.name           = 'Ktoonsez TouchWiz kernel'
file_info.ramdisk        = 'jflte/TouchWiz/TouchWiz.def'
file_info.patch          = autopatcher.auto_patch
file_info.extract        = autopatcher.files_to_auto_patch
