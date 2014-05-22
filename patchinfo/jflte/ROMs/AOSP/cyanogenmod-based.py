from multiboot.fileinfo import FileInfo
import multiboot.autopatcher as autopatcher
import multiboot.fileio as fileio

file_info = FileInfo()

filename_regex           = r"^cm-[0-9\.]+-[0-9]+-.*.zip$"
file_info.name           = 'CyanogenMod-based ROM'
file_info.ramdisk        = 'jflte/AOSP/AOSP.def'
file_info.patch          = autopatcher.auto_patch
file_info.extract        = autopatcher.files_to_auto_patch

def matches(filename):
  return fileio.filename_matches(filename, filename_regex) \
    and 'noobdev' not in filename
