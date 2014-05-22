from multiboot.fileinfo import FileInfo
import multiboot.autopatcher as autopatcher
import multiboot.fileio as fileio

file_info = FileInfo()

filename_regex           = r"^pa_[a-z0-9]+-.+-.+\.zip$"
file_info.name           = 'ParanoidAndroid'
file_info.ramdisk        = 'jflte/AOSP/AOSP.def'
file_info.patch          = autopatcher.auto_patch
file_info.extract        = autopatcher.files_to_auto_patch

def matches(filename):
  return fileio.filename_matches(filename, filename_regex) \
    and 'gapps' not in filename
