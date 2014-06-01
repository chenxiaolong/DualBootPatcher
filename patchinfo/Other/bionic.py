from multiboot.patchinfo import PatchInfo
import multiboot.autopatcher as autopatcher
import os

patchinfo = PatchInfo()

patchinfo.name           = "Moto X's Qualcomm-optimized Dalvik and Bionic libraries"
patchinfo.patch          = autopatcher.auto_patch
patchinfo.extract        = autopatcher.files_to_auto_patch
patchinfo.has_boot_image = False

def matches(filename):
  filename = os.path.split(filename)[1]
  if filename == '4.4_qc-optimized_bionic.zip' or \
      filename == '4.4_qc-optimized_dalvik.zip' or \
      filename == 'KRT16S_stock_revert.zip':
    return True
  else:
    return False

patchinfo.matches        = matches
