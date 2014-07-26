from multiboot.patchinfo import PatchInfo
import multiboot.autopatcher as autopatcher
import multiboot.fileio as fileio
import os
import re

patchinfo = PatchInfo()

patchinfo.matches        = r"^TriForceROM[0-9\.]+Update\.zip$"
patchinfo.name           = 'TriForceROM Update'
patchinfo.ramdisk        = 'jflte/TouchWiz/TouchWiz.def'
patchinfo.patch          = [ autopatcher.auto_patch ]
patchinfo.extract        = autopatcher.files_to_auto_patch

def fix_aroma(directory, bootimg = None, device_check = True,
              partition_config = None, device = None):
  updater_script = 'META-INF/com/google/android/updater-script'
  lines = fileio.all_lines(os.path.join(directory, updater_script))

  i = 0
  while i < len(lines):
    if re.search('getprop.*/system/build.prop', lines[i]):
      i += autopatcher.insert_mount_system(i, lines)
      i += autopatcher.insert_mount_cache(i, lines)
      i += autopatcher.insert_mount_data(i, lines)
      lines[i] = re.sub('/system', partition_config.target_system, lines[i])

    i += 1

  fileio.write_lines(updater_script, lines, directory = directory)

patchinfo.patch.append(fix_aroma)
