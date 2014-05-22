from multiboot.fileinfo import FileInfo
import multiboot.autopatcher as autopatcher

file_info = FileInfo()

filename_regex           = r"^gapps-kk-[0-9]{8}\.zip$"
file_info.name           = 'Gummy Google Apps'
file_info.patch          = autopatcher.auto_patch
file_info.extract        = autopatcher.files_to_auto_patch
file_info.has_boot_image = False
