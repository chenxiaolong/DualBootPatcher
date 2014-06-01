from multiboot.patchinfo import PatchInfo
import multiboot.autopatcher as autopatcher
import multiboot.fileio as fileio

patchinfo = PatchInfo()

patchinfo.name           = 'ParanoidAndroid'
patchinfo.ramdisk        = 'jflte/AOSP/AOSP.def'
patchinfo.patch          = autopatcher.auto_patch
patchinfo.extract        = autopatcher.files_to_auto_patch

def matches(filename):
  regex = r"^pa_[a-z0-9]+-.+-.+\.zip$"
  return fileio.filename_matches(regex, filename) \
    and 'gapps' not in filename

patchinfo.matches        = matches
