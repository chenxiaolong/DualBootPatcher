from multiboot.fileinfo import FileInfo
import multiboot.autopatcher as autopatcher

file_info = FileInfo()

filename_regex           = r"^(Liquid|LS)-(JB|KK|Kitkat)-v[0-9\.]+-.*-jflte.*\.zip$"
file_info.name           = 'LiquidSmooth'
file_info.ramdisk        = 'jflte/AOSP/AOSP.def'
file_info.patch          = autopatcher.auto_patch
file_info.extract        = autopatcher.files_to_auto_patch
