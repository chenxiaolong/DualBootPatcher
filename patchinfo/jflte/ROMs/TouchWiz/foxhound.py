from multiboot.fileinfo import FileInfo
import multiboot.autopatcher as autopatcher

file_info = FileInfo()

filename_regex           = r"^FoxHound_.*\.zip$"
file_info.name           = 'FoxHound'
file_info.ramdisk        = 'jflte/TouchWiz/TouchWiz.def'
file_info.bootimg        = "snakes/Kernels/Stock/boot.img"
file_info.patch          = [ autopatcher.auto_patch,
                             'jflte/ROMs/TouchWiz/foxhound.dualboot.patch' ]
file_info.extract        = [ autopatcher.files_to_auto_patch,
                             'META-INF/com/google/android/aroma-config' ]
