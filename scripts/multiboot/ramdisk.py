import multiboot.config as config
import multiboot.debug as debug
import multiboot.fileio as fileio
import multiboot.patch as patch
import multiboot.operatingsystem as OS

import imp
import os
import re

error_msg = None
suffix = re.compile(r"\.def$")


def process_def(def_file, cpiofile, partition_config):
    debug.debug("Loading ramdisk definition %s" % def_file)

    for line in fileio.all_lines(def_file):
        if line.startswith("pyscript"):
            path = os.path.join(OS.ramdiskdir,
                                re.search(r"^pyscript\s*=\s*\"?(.*)\"?\s*$", line).group(1))

            debug.debug("Loading pyscript " + path)

            plugin = imp.load_source(os.path.basename(path)[:-3],
                                     os.path.join(OS.ramdiskdir, path))

            plugin.patch_ramdisk(cpiofile, partition_config)

    return True


def list_ramdisks():
    deviceramdiskdir = os.path.join(OS.ramdiskdir, config.get_device())

    ramdisks = []

    for root, dirs, files in os.walk(deviceramdiskdir):
        relative_path = os.path.relpath(root, OS.ramdiskdir)
        for f in files:
            if suffix.search(f):
                ramdisks.append(os.path.join(relative_path, f))

    return ramdisks
