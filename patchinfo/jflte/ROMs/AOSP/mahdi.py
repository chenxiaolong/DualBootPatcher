# Thanks to visanciprian for this file!
# http://forum.xda-developers.com/showpost.php?p=49218687&postcount=1733

from multiboot.fileinfo import FileInfo
import multiboot.autopatcher as autopatcher

file_info = FileInfo()

filename_regex           = r"^mahdi-.*jflte.*\.zip$"
file_info.name           = 'Mahdi'
file_info.ramdisk        = 'jflte/AOSP/AOSP.def'
file_info.patch          = autopatcher.auto_patch
file_info.extract        = autopatcher.files_to_auto_patch
