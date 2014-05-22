from multiboot.fileinfo import FileInfo
import multiboot.autopatcher as autopatcher

file_info = FileInfo()

filename_regex           = r"^aokp_hammerhead_.*\.zip$"
file_info.name           = 'AOKP'
file_info.ramdisk        = 'hammerhead/AOSP/AOSP.def'
file_info.patch          = autopatcher.auto_patch
file_info.extract        = autopatcher.files_to_auto_patch
