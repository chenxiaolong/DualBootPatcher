from multiboot.fileinfo import FileInfo
import multiboot.autopatcher as autopatcher
import multiboot.fileio as fileio

file_info = FileInfo()

filename_regex           = r"^negalite-.*\.zip"
file_info.name           = 'Negalite'
file_info.ramdisk        = 'jflte/TouchWiz/TouchWiz.def'
file_info.patch          = [ autopatcher.auto_patch,
                             'jflte/ROMs/TouchWiz/negalite.dualboot.patch' ]
file_info.extract        = [ autopatcher.files_to_auto_patch,
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

file_info.patch.append(dont_wipe_data)
