from multiboot.fileinfo import FileInfo
import multiboot.autopatcher as autopatcher
import multiboot.fileio as fileio
import os

file_info = FileInfo()

# Didn't check Illusion
# Just so I don't need to redownload, illusion filenames:
#   Echoe_Illusion_RomTHEMED_v7_09Nov_Vzw_v2.zip
#   AT&T_Echoe_Illusion_Rom_v7_09Nov.zip
filename_regex           = [ r"^Echoe[ _]?(Rom|SLIM).*\.zip$",
                             r"^S4_Echoe.*\.zip",
                             r'^Echoe_v1[478].*\.zip',
                             r'^S4_Echoe_v2[0].*\.zip' ]
file_info.name           = 'Echoe TouchWiz'
file_info.ramdisk        = 'jflte/TouchWiz/TouchWiz.def'
file_info.patch          = autopatcher.auto_patch
file_info.extract        = autopatcher.files_to_auto_patch

def matches(filename):
  for regex in filename_regex:
    if fileio.filename_matches(filename, regex):
      return True
  return False
