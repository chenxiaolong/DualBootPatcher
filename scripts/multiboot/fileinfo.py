import multiboot.debug as debug
import multiboot.operatingsystem as OS

import imp
import os


class FileInfo:
    def __init__(self):
        self.name = ''
        self.patch = None
        self.extract = None
        self.ramdisk = ""
        self.bootimg = "boot.img"
        self.has_boot_image = True
        self.loki = False
        self.device_check = True
        # Supported partition configurations
        self.configs = ['all']
        self.patched_init = None


def get_info(path, device):
    filename = os.path.split(path)[1]

    for i in ['Google_Apps', 'Other', device]:
        for root, dirs, files in os.walk(os.path.join(OS.patchinfodir, i)):
            for f in files:
                if f.endswith(".py"):
                    plugin = imp.load_source(os.path.basename(f)[:-3],
                                             os.path.join(root, f))
                    if plugin.matches(filename):
                        try:
                            file_info = plugin.get_file_info(filename=filename)
                        except:
                            file_info = plugin.get_file_info()

                        if file_info:
                            debug.debug("Loading patchinfo plugin: " + filename)
                            print('Detected ' + file_info.name)
                            return file_info

    return None


def get_infos(path, device):
    filename = os.path.split(path)[1]

    infos = list()

    for i in ['Google_Apps', 'Other', device]:
        for root, dirs, files in os.walk(os.path.join(OS.patchinfodir, i)):
            for f in files:
                if f.endswith(".py"):
                    relpath = os.path.relpath(os.path.join(root, f), OS.patchinfodir)
                    plugin = imp.load_source(os.path.basename(f)[:-3],
                                             os.path.join(root, f))
                    try:
                        file_info = plugin.get_file_info(filename=filename)
                    except:
                        file_info = plugin.get_file_info()

                    infos.append((relpath, file_info))

    return infos


def get_inits():
    initdir = os.path.join(OS.ramdiskdir, 'init')
    inits = list()

    for root, dirs, files in os.walk(initdir):
        for f in files:
            inits.append(os.path.relpath(os.path.join(root, f), initdir))

    # Newest first
    inits.sort()
    inits.reverse()

    return inits
