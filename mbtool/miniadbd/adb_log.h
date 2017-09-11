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

#pragma once

#include "mblog/logging.h"

#define LOG_TAG "mbtool/miniadbd"

enum AdbComponents : int
{
    ADB_LLIO    = 0x1u,  // Low-level I/O
    ADB_CONN    = 0x2u,  // Connections
    ADB_SERV    = 0x4u,  // Services
    ADB_FDEV    = 0x8u,  // FD events
    ADB_USB     = 0x10u, // USB
    ADB_SOCK    = 0x20u, // Sockets
    ADB_TSPT    = 0x40u, // Transports
};

#define ADB_MASK_ALL \
        ( ADB_LLIO | ADB_CONN | ADB_SERV | ADB_FDEV | ADB_USB | ADB_SOCK | \
          ADB_TSPT )

extern unsigned int adb_log_mask;

#define ADB_SHOULD_LOG(component) \
        (adb_log_mask == 0 || (adb_log_mask & component))

// vararg variants

#define ADB_LOGE(component, ...)            \
    do {                                    \
        if (ADB_SHOULD_LOG(component)) {    \
            LOGE(__VA_ARGS__);              \
        }                                   \
    } while (0)
#define ADB_LOGW(component, ...)            \
    do {                                    \
        if (ADB_SHOULD_LOG(component)) {    \
            LOGW(__VA_ARGS__);              \
        }                                   \
    } while (0)
#define ADB_LOGI(component, ...)            \
    do {                                    \
        if (ADB_SHOULD_LOG(component)) {    \
            LOGI(__VA_ARGS__);              \
        }                                   \
    } while (0)
#define ADB_LOGD(component, ...)            \
    do {                                    \
        if (ADB_SHOULD_LOG(component)) {    \
            LOGD(__VA_ARGS__);              \
        }                                   \
    } while (0)
#define ADB_LOGV(component, ...)            \
    do {                                    \
        if (ADB_SHOULD_LOG(component)) {    \
            LOGV(__VA_ARGS__);              \
        }                                   \
    } while (0)

// va_list variants

#define ADB_VLOGE(component, ...)           \
    do {                                    \
        if (ADB_SHOULD_LOG(component)) {    \
            VLOGE(__VA_ARGS__);             \
        }                                   \
    } while (0)
#define ADB_VLOGW(component, ...)           \
    do {                                    \
        if (ADB_SHOULD_LOG(component)) {    \
            VLOGW(__VA_ARGS__);             \
        }                                   \
    } while (0)
#define ADB_VLOGI(component, ...)           \
    do {                                    \
        if (ADB_SHOULD_LOG(component)) {    \
            VLOGI(__VA_ARGS__);             \
        }                                   \
    } while (0)
#define ADB_VLOGD(component, ...)           \
    do {                                    \
        if (ADB_SHOULD_LOG(component)) {    \
            VLOGD(__VA_ARGS__);             \
        }                                   \
    } while (0)
#define ADB_VLOGV(component, ...)           \
    do {                                    \
        if (ADB_SHOULD_LOG(component)) {    \
            VLOGV(__VA_ARGS__);             \
        }                                   \
    } while (0)
