/*
 * Copyright (C) 2007 The Android Open Source Project
 * Copyright (C) 2015 Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include <string>
#include <unordered_map>

#include <sys/stat.h>

struct BlockDevInfo
{
    std::string path;           // Path to block device
    std::string partition_name; // Partition name (system, cache, data, etc.)
    int partition_num = -1;     // Partition number
    int major = -1;             // Block device major number
    int minor = -1;             // Block device minor number
};

void handle_device_fd();
void device_init(bool dry_run);
void device_close();
int get_device_fd();

std::unordered_map<std::string, BlockDevInfo> get_block_dev_mappings();
