from multiboot.autopatchers.base import BasePatcher
from multiboot.autopatchers.standard import StandardPatcher
from multiboot.patchinfo import PatchInfo
import multiboot.fileio as fileio
import os
import re

patchinfo = PatchInfo()

patchinfo.matches        = r"^TriForceROM[0-9\.]+Update\.zip$"
patchinfo.name           = 'TriForceROM Update'
patchinfo.ramdisk        = 'jflte/TouchWiz/TouchWiz.def'
patchinfo.autopatchers   = [StandardPatcher]


class FixAroma(BasePatcher):
    def __init__(self, **kwargs):
        super(FixAroma, self).__init__(**kwargs)

    def patch(self, directory, file_info, bootimages=None):
        updater_script = 'META-INF/com/google/android/updater-script'
        lines = fileio.all_lines(os.path.join(directory, updater_script))

        i = 0
        while i < len(lines):
            if re.search('getprop.*/system/build.prop', lines[i]):
                i += StandardPatcher.insert_mount_system(i, lines)
                i += StandardPatcher.insert_mount_cache(i, lines)
                i += StandardPatcher.insert_mount_data(i, lines)
                lines[i] = re.sub('/system', file_info.partconfig.target_system, lines[i])

            i += 1

        fileio.write_lines(os.path.join(directory, updater_script), lines)


patchinfo.autopatchers.append(FixAroma)
