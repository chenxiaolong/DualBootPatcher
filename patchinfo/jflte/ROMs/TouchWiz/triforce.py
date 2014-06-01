from multiboot.patchinfo import PatchInfo
import multiboot.autopatcher as autopatcher
import multiboot.fileio as fileio
import re

patchinfo = PatchInfo()

patchinfo.matches        = r"^TriForceROM[0-9\.]+\.zip$"
patchinfo.name           = 'TriForceROM'
patchinfo.ramdisk        = 'jflte/TouchWiz/TouchWiz.def'
patchinfo.patch          = [ autopatcher.auto_patch ]
patchinfo.extract        = [ autopatcher.files_to_auto_patch,
                             'META-INF/com/google/android/aroma-config' ]

def fix_aroma(directory, bootimg = None, device_check = True,
              partition_config = None, device = None):
  aroma_config = 'META-INF/com/google/android/aroma-config'
  lines = fileio.all_lines(aroma_config, directory = directory)

  i = 0
  while i < len(lines):
    if re.search('/system/build.prop', lines[i]):
      # Remove 'raw-' since aroma mounts the partitions directly
      target_dir = re.sub("raw-", "", partition_config.target_system)
      lines[i] = re.sub('/system', target_dir, lines[i])

    elif re.search(r"/sbin/mount.*/system", lines[i]):
      i += autopatcher.insert_line(i + 1, re.sub('/system', '/cache', lines[i]), lines)
      i += autopatcher.insert_line(i + 1, re.sub('/system', '/data', lines[i]), lines)

    elif '~welcome.title' in lines[i]:
      i += autopatcher.insert_line(i + 1, '"***IMPORTANT***: You MUST choose the stock kernel for dual booting to work. If you want to use a custom kernel, you can patch and flash it afterwards.\\n\\n" +', lines)

    i += 1

  fileio.write_lines(aroma_config, lines, directory = directory)

patchinfo.patch.append(fix_aroma)
