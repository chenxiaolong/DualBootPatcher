from multiboot.patchinfo import PatchInfo
import multiboot.autopatcher as autopatcher
import multiboot.fileio as fileio
import re

patchinfo = PatchInfo()

patchinfo.matches        = r"^Slim_aroma_selectable_gapps.*\.zip$"
patchinfo.name           = 'SlimRoms AROMA Google Apps'
patchinfo.patch          = [ autopatcher.auto_patch ]
patchinfo.extract        = autopatcher.files_to_auto_patch
patchinfo.has_boot_image = False

def handle_bundled_mount(directory, bootimg = None, device_check = True,
                         partition_config = None, device = True):
  updater_script = 'META-INF/com/google/android/updater-script'
  lines = fileio.all_lines(updater_script, directory = directory)

  i = 0
  while i < len(lines):
    if re.search('/tmp/mount.*/system', lines[i]):
      del lines[i]
      i += autopatcher.insert_mount_system(i, lines)

    elif re.search('/tmp/mount.*/cache', lines[i]):
      del lines[i]
      i += autopatcher.insert_mount_cache(i, lines)

    elif re.search('/tmp/mount.*/data', lines[i]):
      del lines[i]
      i += autopatcher.insert_mount_data(i, lines)

    else:
      i += 1

  fileio.write_lines(updater_script, lines, directory = directory)

patchinfo.patch.append(handle_bundled_mount)
