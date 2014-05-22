from multiboot.fileinfo import FileInfo
import multiboot.autopatcher as autopatcher

file_info = FileInfo()

filename_regex           = r"^KT-SGS4-[^-]+-AOSP-.*\.zip$"
file_info.name           = 'Ktoonsez AOSP kernel'
file_info.ramdisk        = 'jflte/AOSP/ktoonsez.def'
file_info.patch          = autopatcher.auto_patch
file_info.extract        = autopatcher.files_to_auto_patch
