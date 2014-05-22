from multiboot.fileinfo import FileInfo

file_info = FileInfo()

filename_regex           = r"^ComaDose_V[0-9\.]+_Cossbreeder_[0-9\.]+\.zip"
file_info.name           = 'ComaDose'
file_info.patch          = 'jflte/Other/comadose.dualboot.patch'
file_info.has_boot_image = False
