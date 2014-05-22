from multiboot.fileinfo import FileInfo

file_info = FileInfo()

filename_regex           = r"^UPDATE-SuperSU-v[0-9\.]+\.zip$"
file_info.name           = "Chainfire's SuperSU"
file_info.patch          = 'Other/supersu.dualboot.patch'
file_info.has_boot_image = False
