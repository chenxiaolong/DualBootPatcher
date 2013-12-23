import multiboot.debug as debug
import multiboot.fileio as fileio
import multiboot.patch as patch
import multiboot.operatingsystem as OS

import imp
import os
import re

error_msg = None


def process_def(def_file, directory, partition_config):
    debug.debug("Loading ramdisk definition %s in directory %s" % (def_file, directory))

    for line in fileio.all_lines(def_file):
        if line.startswith("pyscript"):
            path = os.path.join(OS.ramdiskdir,
                                re.search(r"^pyscript\s*=\s*\"?(.*)\"?\s*$", line).group(1))

            debug.debug("Loading pyscript " + path)

            plugin = imp.load_source(os.path.basename(path)[:-3],
                                     os.path.join(OS.ramdiskdir, path))

            plugin.patch_ramdisk(directory, partition_config)

        elif line.startswith("patch"):
            path = os.path.join(OS.ramdiskdir,
                                re.search(r"^patch\s*=\s*\"?(.*)\"?\s*$", line).group(1))
            if not patch.apply_patch(path, directory):
                error_msg = patch.error_msg
                return False

    return True
