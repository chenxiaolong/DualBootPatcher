from multiboot.fileinfo import FileInfo
import os

file_info = FileInfo()

file_info.name           = 'iNTriNsiC'
file_info.ramdisk        = 'jflte/TouchWiz/TouchWiz.def'
file_info.patch          = 'jflte/ROMs/TouchWiz/intrinsic-20130806.dualboot.patch'

def matches(filename):
  filename = os.path.split(filename)[1]
  return filename == "iNTriNsiC 8-6-13.zip"
