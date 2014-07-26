from multiboot.patchinfo import PatchInfo
import multiboot.autopatcher as autopatcher
import multiboot.fileio as fileio
import os

patchinfo = PatchInfo()

patchinfo.matches        = r'^Imperium_.*\.zip$'
patchinfo.name           = 'Imperium'
patchinfo.ramdisk        = 'jflte/TouchWiz/TouchWiz.def'
patchinfo.extract        = autopatcher.files_to_auto_patch

def auto_patch(directory, bootimg=None, device_check=True,
               partition_config=None, device=None):
    updater_script = 'META-INF/com/google/android/updater-script'

    lines = fileio.all_lines(os.path.join(directory, updater_script))

    autopatcher.insert_dual_boot_sh(lines)
    autopatcher.replace_mount_lines(device, lines)
    autopatcher.replace_unmount_lines(device, lines)
    autopatcher.replace_format_lines(device, lines)
    autopatcher.insert_unmount_everything(len(lines), lines)

    # Insert set kernel line
    set_kernel_line = 'run_program("/tmp/dualboot.sh", "set-multi-kernel");'
    i = len(lines) - 1
    while i > 0:
        if 'Umounting Partitions' in lines[i]:
            autopatcher.insert_line(i + 1, set_kernel_line, lines)
            break

        i -= 1

    fileio.write_lines(updater_script, lines, directory=directory)

patchinfo.patch          = auto_patch
