from multiboot.fileinfo import FileInfo
import multiboot.autopatcher as autopatcher
import re

file_info = FileInfo()

# Didn't check Illusion
# Just so I don't need to redownload, illusion filenames:
#   Echoe_Illusion_RomTHEMED_v7_09Nov_Vzw_v2.zip
#   AT&T_Echoe_Illusion_Rom_v7_09Nov.zip
filename_regex           = [ r"^Echoe[ _]?(Rom|SLIM).*\.zip$",
                             r"^S4_Echoe.*\.zip",
                             r'^Echoe_v1[478].*\.zip' ]
file_info.name           = 'Echoe TouchWiz'
file_info.ramdisk        = 'jflte/TouchWiz/TouchWiz.def'
file_info.patch          = autopatcher.auto_patch
file_info.extract        = autopatcher.files_to_auto_patch
file_info.bootimg        = 'boot.img'

def matches(filename):
  for regex in filename_regex:
    if re.search(regex, filename):
      return True
  return False

def get_file_info(filename = ""):
  if 'Echoe_v17' in filename:
    file_info.bootimg = [
      'EchoeExtras/kernel/echoe1/boot.img',
      'EchoeExtras/kernel/echoe2/boot.img',
      'EchoeExtras/kernel/KT/boot.img',
      'EchoeExtras/kernel/stock/boot.img',
      'EchoeExtras/kernel/talex/boot.img'
    ]
  elif 'Echoe_v18' in filename:
    file_info.bootimg = [
      'EchoeExtras/kernel/adam/boot.img',
      'EchoeExtras/kernel/echoe1/boot.img',
      'EchoeExtras/kernel/echoe2/boot.img',
      'EchoeExtras/kernel/KT/boot.img',
      'EchoeExtras/kernel/stock/boot.img',
      'EchoeExtras/kernel/talex/boot.img'
    ]
  return file_info
