from multiboot.fileinfo import FileInfo
import multiboot.autopatcher as autopatcher

file_info = FileInfo()

filename_regex           = r"^cm-[0-9\.]+(-[0-9]+-NIGHTLY)?-[a-z0-9]+\.zip$"
file_info.name           = 'Official CyanogenMod'
file_info.ramdisk        = 'hammerhead/AOSP/AOSP.def'
file_info.patch          = autopatcher.auto_patch
file_info.extract        = autopatcher.files_to_auto_patch
