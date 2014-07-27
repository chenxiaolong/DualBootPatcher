from multiboot.autopatchers.base import BasePatcher
from multiboot.autopatchers.cyanogenmod import DalvikCachePatcher
from multiboot.autopatchers.standard import StandardPatcher
from multiboot.patchinfo import PatchInfo
import multiboot.fileio as fileio
import os

patchinfo = PatchInfo()

patchinfo.matches        = r"^cm-.*noobdev.*.zip$"
patchinfo.name           = "chenxiaolong's noobdev CyanogenMod"
patchinfo.ramdisk        = 'jflte/AOSP/cxl.def'
patchinfo.autopatchers   = [StandardPatcher, DalvikCachePatcher]
# ROM has built in dual boot support
patchinfo.configs        = ['all', '!dual']


class MultiBoot(BasePatcher):
    def __init__(self, **kwargs):
        super(MultiBoot, self).__init__(**kwargs)

        # Don't need to add, since it's not from the original zip file
        #self.files_list.append('dualboot.sh')

    def patch(self, directory, file_info, bootimages=None):
        # My ROM has dual-boot support built in, so we'll hack around that to
        # support triple, quadruple, ... boot
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

        fileio.write_lines(os.path.join(directory, updater_script), lines)

        # Create /tmp/dualboot.prop
        lines = fileio.all_lines(os.path.join(directory, 'dualboot.sh'))

        lines.append("echo 'ro.dualboot=0' > /tmp/dualboot.prop")

        fileio.write_lines(os.path.join(directory, 'dualboot.sh'), lines)


class SystemProp(BasePatcher):
    def __init__(self, **kwargs):
        super(SystemProp, self).__init__(**kwargs)

        self.files_list.append('system/build.prop')

    def patch(self, directory, file_info, bootimages=None):
        # The auto-updater in my ROM needs to know if the ROM has been patched
        lines = fileio.all_lines(os.path.join(directory, 'system/build.prop'))

        lines.append('ro.chenxiaolong.patched=%s\n' % file_info.partconfig.id)

        fileio.write_lines(os.path.join(directory, 'system/build.prop'), lines)


patchinfo.autopatchers.insert(0, MultiBoot)
patchinfo.autopatchers.append(SystemProp)
