import multiboot.cmd as cmd
import multiboot.debug as debug
import multiboot.fileio as fileio
import multiboot.operatingsystem as OS

import os
import re

output = None
error = None
error_msg = None


def apply_patch(patch_file, directory):
    global output, error, error_msg

    debug.debug("Applying patch: %s in directory %s" % (patch_file, directory))

    exit_code, output, error = cmd.run_command(
        [OS.patch,
         '--no-backup-if-mismatch',
         '-p', '1',
         '-d', directory,
         '-i', os.path.join(OS.patchdir, patch_file)]
    )

    if exit_code != 0:
        error_msg = "Failed to apply patch"
        return False
    else:
        return True


def files_in_patch(patch_file):
    global error_msg

    files = []

    lines = fileio.all_lines(patch_file, directory=OS.patchdir)

    counter = 0
    while counter < len(lines):
        if re.search(r"^---", lines[counter]) \
                and re.search(r"^\+\+\+", lines[counter + 1]) \
                and re.search(r"^@",      lines[counter + 2]):
            temp = re.search(r"^--- .*?/(.*)$", lines[counter])
            debug.debug("Found in patch %s: %s" % (patch_file, temp.group(1)))
            files.append(temp.group(1))
            counter += 3
        else:
            counter += 1

    if not files:
        error_msg = "Failed to read list of files in patch"

    return files
