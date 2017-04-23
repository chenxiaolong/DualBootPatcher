/*
 * Copyright (C) 2007 The Android Open Source Project
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

#include <stddef.h>
#include <sys/socket.h>
#include <string>

#include "mbutil/external/system_properties.h"

struct property_audit_data {
    ucred *cr;
    const char* name;
};

bool property_init();
void property_load_boot_defaults();
void load_persist_props();
void load_system_props();
bool start_property_service();
bool stop_property_service();
std::string property_get(const char* name);
uint32_t property_set(const std::string& name, const std::string& value);
bool is_legal_property_name(const std::string& name);
