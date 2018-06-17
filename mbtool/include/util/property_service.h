/*
 * Copyright (C) 2007 The Android Open Source Project
 * Copyright (C) 2018 Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include <optional>
#include <string>
#include <string_view>
#include <thread>

#include "mbcommon/common.h"

class SocketConnection;

class PropertyService
{
public:
    PropertyService();
    ~PropertyService();

    MB_DISABLE_COPY_CONSTRUCT_AND_ASSIGN(PropertyService)
    MB_DISABLE_MOVE_CONSTRUCT_AND_ASSIGN(PropertyService)

    bool initialize();

    std::optional<std::string> get(const std::string &name);
    bool set(const std::string &name, std::string_view value);

    bool start_thread();
    bool stop_thread();

    void load_boot_props();
    void load_system_props();

    bool load_properties_file(const std::string &path, std::string_view filter);

private:
    bool m_initialized;
    int m_setter_fd;
    int m_stopper_pipe[2];
    std::thread m_thread;

    uint32_t set_internal(const std::string &name, std::string_view value);

    int create_socket();

    void socket_handler_loop();
    void socket_handle_set_property();
    void socket_handle_set_property_impl(SocketConnection &socket,
                                         const std::string &name,
                                         std::string_view value, bool legacy);
};
