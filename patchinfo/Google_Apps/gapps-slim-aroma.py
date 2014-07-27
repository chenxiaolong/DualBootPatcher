from multiboot.autopatchers.base import BasePatcher
from multiboot.autopatchers.standard import StandardPatcher
from multiboot.patchinfo import PatchInfo
import multiboot.fileio as fileio
import os
import re

patchinfo = PatchInfo()

patchinfo.matches        = r"^Slim_aroma_selectable_gapps.*\.zip$"
patchinfo.name           = 'SlimRoms AROMA Google Apps'
patchinfo.autopatchers   = [StandardPatcher]
patchinfo.has_boot_image = False


class HandleBundledMount(BasePatcher):
    def __init__(self, **kwargs):
        super(HandleBundledMount, self).__init__(**kwargs)

    def patch(self, directory, file_info, bootimages=None):
        updater_script = 'META-INF/com/google/android/updater-script'
        lines = fileio.all_lines(os.path.join(directory, updater_script))

        i = 0
        while i < len(lines):
            if re.search('/tmp/mount.*/system', lines[i]):
                del lines[i]
                i += StandardPatcher.insert_mount_system(i, lines)

            elif re.search('/tmp/mount.*/cache', lines[i]):
                del lines[i]
                i += StandardPatcher.insert_mount_cache(i, lines)

            elif re.search('/tmp/mount.*/data', lines[i]):
                del lines[i]
                i += StandardPatcher.insert_mount_data(i, lines)

            else:
                i += 1

        fileio.write_lines(os.path.join(directory, updater_script), lines)


patchinfo.autopatchers.append(HandleBundledMount)
