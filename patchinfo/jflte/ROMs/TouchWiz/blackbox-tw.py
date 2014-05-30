from multiboot.fileinfo import FileInfo
import multiboot.autopatcher as autopatcher
import multiboot.fileio as fileio

file_info = FileInfo()

filename_regex           = r"^[a-zA-Z0-9]+BlackBox[0-9]+SGS4\.zip$"
file_info.name           = 'BlackBox TouchWiz'
file_info.ramdisk        = 'jflte/TouchWiz/TouchWiz.def'
file_info.patch          = autopatcher.auto_patch
file_info.extract        = autopatcher.files_to_auto_patch
