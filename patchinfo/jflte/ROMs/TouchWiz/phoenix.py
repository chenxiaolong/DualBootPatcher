from multiboot.fileinfo import FileInfo
import multiboot.autopatcher as autopatcher

file_info = FileInfo()

filename_regex           = r"^PhoeniX.ROM.*\.zip$"
file_info.name           = 'PhoeniX'
file_info.ramdisk        = 'jflte/TouchWiz/TouchWiz.def'
file_info.patch          = autopatcher.auto_patch
file_info.extract        = autopatcher.files_to_auto_patch
