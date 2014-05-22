from multiboot.fileinfo import FileInfo
import multiboot.autopatcher as autopatcher

file_info = FileInfo()

filename_regex           = r'^EclipseTW.*\.zip$'
file_info.name           = 'Eclipse TouchWiz'
file_info.ramdisk        = 'jflte/TouchWiz/TouchWiz.def'
file_info.patch          = autopatcher.auto_patch
file_info.extract        = autopatcher.files_to_auto_patch
file_info.loki           = True
file_info.bootimg        = [
  'eclipse/kernels/kt/boot.lok',
  'eclipse/kernels/stock/boot.lok'
]
