class PartitionConfig:
  # Location, filesystem, and mount point of the raw partitions
  dev_system="/dev/block/platform/msm_sdcc.1/by-name/system::ext4::/raw-system"
  dev_cache="/dev/block/platform/msm_sdcc.1/by-name/cache::ext4::/raw-cache"
  dev_data="/dev/block/platform/msm_sdcc.1/by-name/userdata::ext4::/raw-data"

  SYSTEM="$DEV_SYSTEM"
  CACHE="$DEV_CACHE"
  DATA="$DEV_DATA"

  def __init__(self):
    self.name = ""
    self.description = ""
    self.kernel = ""
    self.id = ""

    self.target_system = ""
    self.target_cache = ""
    self.target_data = ""

    self.target_system_partition = ""
    self.target_cache_partition = ""
    self.target_data_partition = ""
