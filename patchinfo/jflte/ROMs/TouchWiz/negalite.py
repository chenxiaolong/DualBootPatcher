from multiboot.autopatchers.base import BasePatcher
from multiboot.autopatchers.standard import StandardPatcher
from multiboot.patchinfo import PatchInfo
import multiboot.fileio as fileio
import os

patchinfo = PatchInfo()

patchinfo.matches        = r"^negalite-.*\.zip"
patchinfo.name           = 'Negalite'
patchinfo.ramdisk        = 'jflte/TouchWiz/TouchWiz.def'
patchinfo.autopatchers   = [StandardPatcher,
                            'jflte/ROMs/TouchWiz/negalite.dualboot.patch']


class DontWipeData(BasePatcher):
    def __init__(self, **kwargs):
        super(DontWipeData, self).__init__(**kwargs)

    def patch(self, directory, file_info, bootimages=None):
        updater_script = 'META-INF/com/google/android/updater-script'
        lines = fileio.all_lines(os.path.join(directory, updater_script))

        i = 0
        while i < len(lines):
            if re.search('run_program.*/tmp/wipedata.sh', lines[i]):
                del lines[i]
                StandardPatcher.insert_format_data(i, lines)
                break

            i += 1

        fileio.write_lines(os.path.join(directory, updater_script), lines)


patchinfo.autopatchers.append(DontWipeData)
