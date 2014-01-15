from multiboot.fileinfo import FileInfo

file_info = FileInfo()

file_info.name           = 'iNTriNsiC'
file_info.ramdisk        = 'jflte/TouchWiz/TouchWiz.def'
file_info.patch          = 'jflte/ROMs/TouchWiz/intrinsic-20130806.dualboot.patch'

def matches(filename):
  if filename == "iNTriNsiC 8-6-13.zip":
    return True
  else:
    return False

def get_file_info():
  return file_info
