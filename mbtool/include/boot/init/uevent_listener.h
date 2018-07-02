/*
 * Copyright (C) 2017 The Android Open Source Project
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

#include <chrono>
#include <functional>
#include <optional>

#include <dirent.h>

#include "boot/init/uevent.h"
#include "boot/init/unique_fd.h"

#define UEVENT_MSG_LEN 2048

namespace android {
namespace init {

enum class ListenerAction {
    kStop = 0,  // Stop regenerating uevents as we've handled the one(s) we're interested in.
    kContinue,  // Continue regenerating uevents as we haven't seen the one(s) we're interested in.
};

using ListenerCallback = std::function<ListenerAction(const Uevent&)>;

class UeventListener {
  public:
    UeventListener();

    void RegenerateUevents(const ListenerCallback& callback) const;
    ListenerAction RegenerateUeventsForPath(const std::string& path,
                                            const ListenerCallback& callback) const;
    void Poll(const ListenerCallback& callback, int cancel_fd,
              const std::optional<std::chrono::milliseconds> relative_timeout = {}) const;

  private:
    bool ReadUevent(Uevent* uevent) const;
    ListenerAction RegenerateUeventsForDir(DIR* d, const ListenerCallback& callback) const;

    android::base::unique_fd device_fd_;
};

}  // namespace init
}  // namespace android
