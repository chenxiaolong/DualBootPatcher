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

#include <cstddef>

#include "mbutil/external/system_properties.h"

bool property_init();
bool property_cleanup();
void property_load_boot_defaults();
void load_system_props();
bool start_property_service();
bool stop_property_service();
void get_property_workspace(int *fd, int *sz);
int __property_get(const char *name, char *value);
int property_set(const char *name, const char *value);
bool property_get_bool(const char *name, bool def_value);
bool properties_initialized();

#ifndef __clang__
void __property_get_size_error()
    __attribute__((__error__("property_get called with too small buffer")));
#else
void __property_get_size_error();
#endif

static inline
__attribute__ ((always_inline))
__attribute__ ((gnu_inline))
#ifndef __clang__
__attribute__ ((artificial))
#endif
int property_get(const char *name, char *value)
{
#ifndef __clang__
    size_t value_len = __builtin_object_size(value, 0);
    if (value_len != PROP_VALUE_MAX)
        __property_get_size_error();
#endif

    return __property_get(name, value);
}
