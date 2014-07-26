from multiboot.patchinfo import PatchInfo
import multiboot.autopatcher as autopatcher
import multiboot.fileio as fileio
from multiboot.autopatchers.cyanogenmod import DalvikCachePatcher
import os

patchinfo = PatchInfo()

patchinfo.matches        = r"^cm-.*noobdev.*.zip$"
patchinfo.name           = "chenxiaolong's noobdev CyanogenMod"
patchinfo.ramdisk        = 'jflte/AOSP/cxl.def'
patchinfo.patch          = [ autopatcher.auto_patch, DalvikCachePatcher.store_in_data ]
patchinfo.extract        = [ autopatcher.files_to_auto_patch, DalvikCachePatcher.files_for_store_in_data, 'system/build.prop' ]
# ROM has built in dual boot support
patchinfo.configs        = [ 'all', '!dual' ]

# My ROM has dual-boot support built in, so we'll hack around that to support
# triple, quadruple, ... boot
def multi_boot(directory, bootimg = None, device_check = True,
               partition_config = None, device = None):
  updater_script = 'META-INF/com/google/android/updater-script'
  lines = fileio.all_lines(os.path.join(directory, updater_script))

  i = 0
  while i < len(lines):
    # Remove existing dualboot.sh lines
    if 'system/bin/dualboot.sh' in lines[i]:
      del lines[i]

    # Remove confusing messages
    elif 'boot installation is' in lines[i]:
      # Can't remove - sole line inside if statement
      lines[i] = 'ui_print("");\n'
      i += 1

    elif 'set-secondary' in lines[i]:
      lines[i] = 'ui_print("");\n'
      i += 1

    else:
      i += 1

  fileio.write_lines(updater_script, lines, directory = directory)

  # Create /tmp/dualboot.prop
  lines = fileio.all_lines(os.path.join(directory, 'dualboot.sh'))

  lines.append("echo 'ro.dualboot=0' > /tmp/dualboot.prop")

  fileio.write_lines('dualboot.sh', lines, directory = directory)

patchinfo.patch.insert(0, multi_boot)

# The auto-updater in my ROM needs to know if the ROM has been patched
def system_prop(directory, bootimg = None, device_check = True,
                partition_config = None, device = None):
  lines = fileio.all_lines(os.path.join(directory, 'system/build.prop'))

  lines.append('ro.chenxiaolong.patched=%s\n' % partition_config.id)

  fileio.write_lines('system/build.prop', lines, directory = directory)

patchinfo.patch.append(system_prop)
