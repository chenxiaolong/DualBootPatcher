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
    self.configs = [ 'all' ]
