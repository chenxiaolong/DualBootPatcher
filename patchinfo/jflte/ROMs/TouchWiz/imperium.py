from multiboot.autopatchers.base import BasePatcher
from multiboot.autopatchers.standard import StandardPatcher
from multiboot.patchinfo import PatchInfo
import multiboot.fileio as fileio
import os

patchinfo = PatchInfo()

patchinfo.matches        = r'^Imperium_.*\.zip$'
patchinfo.name           = 'Imperium'
patchinfo.ramdisk        = 'jflte/TouchWiz/TouchWiz.def'


class ModifiedStandard(BasePatcher):
    def __init__(self, **kwargs):
        super(ModifiedStandard, self).__init__(**kwargs)

        self.files_list.append('META-INF/com/google/android/updater-script')

    def patch(self, directory, file_info, bootimages=None):
        updater_script = 'META-INF/com/google/android/updater-script'

        lines = fileio.all_lines(os.path.join(directory, updater_script))

        StandardPatcher.insert_dual_boot_sh(lines)
        StandardPatcher.replace_mount_lines(file_info.device, lines)
        StandardPatcher.replace_unmount_lines(file_info.device, lines)
        StandardPatcher.replace_format_lines(file_info.device, lines)
        StandardPatcher.insert_unmount_everything(len(lines), lines)

        # Insert set kernel line
        set_kernel_line = 'run_program("/tmp/dualboot.sh", "set-multi-kernel");\n'
        i = len(lines) - 1
        while i > 0:
            if 'Umounting Partitions' in lines[i]:
                lines.insert(i + 1, set_kernel_line)
                break

            i -= 1

        fileio.write_lines(os.path.join(directory, updater_script), lines)


patchinfo.autopatchers   = ModifiedStandard
