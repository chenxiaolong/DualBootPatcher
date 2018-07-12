/*
 * Copyright (C) 2007 The Android Open Source Project
 * Copyright (C) 2015-2018 Andrew Gunnerson <andrewgunnerson@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <algorithm>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <sys/stat.h>
#include <sys/types.h>

#include "mbutil/path.h"

#include "boot/init/uevent.h"

namespace android {
namespace init {

struct BlockDevInfo {
  std::string path;           // Path to block device
  std::string partition_name; // Partition name (system, cache, data, etc.)
  int partition_num = -1;     // Partition number
  int major = -1;             // Block device major number
  int minor = -1;             // Block device minor number
};

using BlockDevMap = std::unordered_map<std::string, BlockDevInfo>;

class DeviceHandler {
  public:
    friend class DeviceHandlerTester;

    DeviceHandler();
    ~DeviceHandler(){};

    void HandleDeviceEvent(const Uevent& uevent);

    std::vector<std::string> GetBlockDeviceSymlinks(const Uevent& uevent) const;

    BlockDevMap GetBlockDeviceMap() const;

  private:
    bool FindPlatformDevice(std::string path, std::string* platform_device_path) const;
    void MakeDevice(const std::string& path, bool block, int major, int minor) const;
    void HandleDevice(const std::string& action, const std::string& devpath, bool block, int major,
                      int minor, const std::vector<std::string>& links) const;

    std::string sysfs_mount_point_;
    mutable std::optional<std::string> boot_device_;

    BlockDevMap block_dev_mappings_;
    mutable std::mutex block_dev_mappings_guard_;
};

// Exposed for testing
void SanitizePartitionName(std::string* string);

}  // namespace init
}  // namespace android
