import multiboot.debug as debug
import multiboot.operatingsystem as OS

import imp
import os


class FileInfo:
    def __init__(self):
        self.loki = False
        self.patch = None
        self.extract = None
        self.ramdisk = ""
        self.bootimg = "boot.img"
        self.has_boot_image = True
        self.need_new_init = False
        self.loki = False
        self.device_check = True
        # Supported partition configurations
        self.configs = ['all']
        # Disable SELinux by default
        self.selinux = False


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
                            plugin.print_message()
                            return file_info

    return None
