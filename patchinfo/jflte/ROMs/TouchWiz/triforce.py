from multiboot.autopatchers.base import BasePatcher
from multiboot.autopatchers.standard import StandardPatcher
from multiboot.patchinfo import PatchInfo
import multiboot.fileio as fileio
import os
import re

patchinfo = PatchInfo()

patchinfo.matches        = r"^TriForceROM[0-9\.]+\.zip$"
patchinfo.name           = 'TriForceROM'
patchinfo.ramdisk        = 'jflte/TouchWiz/TouchWiz.def'
patchinfo.autopatchers   = [StandardPatcher]


class FixAroma(BasePatcher):
    def __init__(self, **kwargs):
        super(FixAroma, self).__init__(**kwargs)

        self.files_list.append('META-INF/com/google/android/aroma-config')

    def patch(self, directory, file_info, bootimages=None):
        aroma_config = 'META-INF/com/google/android/aroma-config'
        lines = fileio.all_lines(os.path.join(directory, aroma_config))

        i = 0
        while i < len(lines):
            if re.search('/system/build.prop', lines[i]):
                # Remove 'raw-' since aroma mounts the partitions directly
                target_dir = re.sub("raw-", "", file_info.partconfig.target_system)
                lines[i] = re.sub('/system', target_dir, lines[i])

            elif re.search(r"/sbin/mount.*/system", lines[i]):
                i += autopatcher.insert_line(i + 1, re.sub('/system', '/cache', lines[i]), lines)
                i += autopatcher.insert_line(i + 1, re.sub('/system', '/data', lines[i]), lines)

            i += 1

        fileio.write_lines(os.path.join(directory, aroma_config), lines)


patchinfo.autopatchers.append(FixAroma)
