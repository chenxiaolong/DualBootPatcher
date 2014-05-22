from multiboot.fileinfo import FileInfo

file_info = FileInfo()

filename_regex           = r"^Evil_UnWizzed_v[0-9]+\.zip$"
file_info.name           = 'Evil UnWizzed'
file_info.ramdisk        = 'jflte/TouchWiz/TouchWiz.def'
file_info.patch          = 'jflte/ROMs/TouchWiz/unwizzed.dualboot.patch'
