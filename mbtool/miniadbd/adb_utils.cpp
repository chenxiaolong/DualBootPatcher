/*
 * Copyright (C) 2015 The Android Open Source Project
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

#include "adb_utils.h"

#include <cstdlib>

#include "adb_log.h"

#include "mbcommon/string.h"

void dump_hex(const void* data, size_t byte_count) {
    byte_count = std::min(byte_count, size_t(16));

    const uint8_t* p = reinterpret_cast<const uint8_t*>(data);

    std::string line;
    for (size_t i = 0; i < byte_count; ++i) {
        char *tmp = mb_format("%02x", p[i]);
        if (tmp) {
            line += tmp;
            free(tmp);
        }
    }
    line.push_back(' ');

    for (size_t i = 0; i < byte_count; ++i) {
        int c = p[i];
        if (c < 32 || c > 127) {
            c = '.';
        }
        line.push_back(c);
    }

    ADB_LOGD(ADB_LLIO, "%s", line.c_str());
}
