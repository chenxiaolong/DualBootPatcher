import multiboot.debug as debug
import multiboot.operatingsystem as OS

import imp
import os


BOOT_IMAGE = 'img'
ZIP_FILE = 'zip'
UNSUPPORTED = None


class FileInfo:
    def __init__(self):
        # Public (to be set by patchfile scripts)
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

        # Private
        self._filename = None
        self._filetype = None # zip, img

    def check_extension(self):
        extension = os.path.splitext(self.filename)[1]
        if extension == '.zip':
            self._filetype = ZIP_FILE
        elif extension == '.img':
            self._filetype = BOOT_IMAGE
        elif extension == '.lok':
            self._filetype = BOOT_IMAGE
        else:
            self._filetype = UNSUPPORTED

    def check_partconfig_supported(self, partconfig):
        if (('all' not in self.configs
                and partconfig.id not in self.configs)
                or ('!' + partconfig.id) in self.configs):
            return False
        else:
            return True



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
                            print('Detected ' + file_info.name)
                            file_info._filename = filename
                            file_info.check_extension()
                            return file_info

    return None


def get_infos(device):
    infos = list()

    for i in ['Google_Apps', 'Other', device]:
        for root, dirs, files in os.walk(os.path.join(OS.patchinfodir, i)):
            for f in files:
                if f.endswith(".py"):
                    relpath = os.path.relpath(os.path.join(root, f), OS.patchinfodir)
                    plugin = imp.load_source(os.path.basename(f)[:-3],
                                             os.path.join(root, f))
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
