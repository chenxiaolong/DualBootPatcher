from fileinfo import FileInfo
import common as c
import re

file_info = FileInfo()

filename_regex           = r"^Dandroid.*\.zip$"
file_info.ramdisk        = 'jflte/GoogleEdition/GoogleEdition.def'
file_info.patch          = [ c.auto_patch ]
file_info.extract        = [ c.files_to_auto_patch, 'system/etc/init.qcom.audio.sh' ]

def matches(filename):
  if re.search(filename_regex, filename):
    return True
  else:
    return False

def print_message():
  print("Detected Dandroid zip")

def get_file_info():
  return file_info

# WTF? Script removes system/etc/snd_soc_msm/snd_soc_msm_2x_Fusion3_auxpcm
# causing the ROM to fail to boot
def unbootable(directory, bootimg = None, device_check = True,
               partition_config = None):
  lines = c.get_lines_from_file(directory, 'system/etc/init.qcom.audio.sh')

  i = 0
  while i < len(lines):
    if 'snd_soc_msm_2x_Fusion3_auxpcm' in lines[i]:
      del lines[i]

    else:
      i += 1

  c.write_lines_to_file(directory, 'system/etc/init.qcom.audio.sh', lines)

file_info.patch.append(unbootable)
