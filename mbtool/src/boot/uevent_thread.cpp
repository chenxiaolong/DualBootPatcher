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

#include "boot/uevent_thread.h"

#include <cerrno>
#include <cstring>

#include <fcntl.h>
#include <unistd.h>

#include "mbcommon/finally.h"

#include "mblog/logging.h"

#define LOG_TAG "mbtool/boot/uevent_thread"

namespace mb
{

using namespace android::init;

UeventThread::UeventThread()
    : m_cancel_pipe()
    , m_is_running(false)
{
}

UeventThread::~UeventThread()
{
    stop();
}

bool UeventThread::start()
{
    if (m_is_running) {
        LOGW("Thread is already running");
        return false;
    }

    if (pipe2(m_cancel_pipe, O_CLOEXEC) < 0) {
        LOGE("Failed to create cancellation pipe: %s", strerror(errno));
        return false;
    }

    auto close_pipe = finally([&] {
        close(m_cancel_pipe[0]);
        close(m_cancel_pipe[1]);
    });

    // Deferred initialization because the constructor opens a socket and aborts
    // if it fails
    m_uevent_listener = UeventListener();

    // Regenerate events for devices already detected
    m_uevent_listener->RegenerateUevents([&](const Uevent &uevent) {
        m_device_handler.HandleDeviceEvent(uevent);
        return ListenerAction::kContinue;
    });

    m_thread = std::thread(&UeventThread::thread_func, this);

    close_pipe.dismiss();
    m_is_running = true;

    LOGV("Started uevent thread");

    return true;
}

bool UeventThread::stop()
{
    if (!m_is_running) {
        LOGW("Thread is not running");
        return false;
    }

    if (write(m_cancel_pipe[1], "", 1) < 0) {
        LOGW("Failed to send cancellation signal");
    }

    m_thread.join();

    // The UeventListener socket will automatically be closed
    m_uevent_listener = std::nullopt;

    close(m_cancel_pipe[0]);
    close(m_cancel_pipe[1]);

    m_is_running = false;

    LOGV("Stopped uevent thread");

    return true;
}

const android::init::DeviceHandler & UeventThread::device_handler() const
{
    return m_device_handler;
}

void UeventThread::thread_func()
{
    m_uevent_listener->Poll([&](const Uevent &uevent) {
        m_device_handler.HandleDeviceEvent(uevent);
        return ListenerAction::kContinue;
    }, m_cancel_pipe[0]);
}

}
