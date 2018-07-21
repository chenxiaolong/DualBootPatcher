/*
 * Copyright (C) 2018  Andrew Gunnerson <andrewgunnerson@gmail.com>
 *
 * This file is part of DualBootPatcher
 *
 * DualBootPatcher is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * DualBootPatcher is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with DualBootPatcher.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <optional>
#include <thread>

#include "boot/init/devices.h"
#include "boot/init/uevent_listener.h"

namespace mb
{

class UeventThread
{
public:
    UeventThread();
    ~UeventThread();

    bool start();
    bool stop();

    const android::init::DeviceHandler & device_handler() const;

private:
    android::init::DeviceHandler m_device_handler;
    // Need deferred initialization because the constructor opens a socket
    std::optional<android::init::UeventListener> m_uevent_listener;

    int m_cancel_pipe[2];

    std::thread m_thread;
    bool m_is_running;

    void thread_func();
};

}
