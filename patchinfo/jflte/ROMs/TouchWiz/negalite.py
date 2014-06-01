from multiboot.patchinfo import PatchInfo
import multiboot.autopatcher as autopatcher
import multiboot.fileio as fileio

patchinfo = PatchInfo()

patchinfo.matches        = r"^negalite-.*\.zip"
patchinfo.name           = 'Negalite'
patchinfo.ramdisk        = 'jflte/TouchWiz/TouchWiz.def'
patchinfo.patch          = [ autopatcher.auto_patch,
                             'jflte/ROMs/TouchWiz/negalite.dualboot.patch' ]
patchinfo.extract        = [ autopatcher.files_to_auto_patch,
                             'META-INF/com/google/android/aroma-config' ]

def dont_wipe_data(directory, bootimg = None, device_check = True,
                   partition_config = None, device = None):
  updater_script = 'META-INF/com/google/android/updater-script'
  lines = fileio.all_lines(updater_script, directory = directory)

  i = 0
  while i < len(lines):
    if re.search('run_program.*/tmp/wipedata.sh', lines[i]):
      del lines[i]
      autopatcher.insert_format_data(i, lines)
      break

    i += 1

  fileio.write_lines(updater_script, lines, directory = directory)

patchinfo.patch.append(dont_wipe_data)
