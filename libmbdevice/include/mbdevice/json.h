/*
 * Copyright (C) 2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
 *
 * This file is part of MultiBootPatcher
 *
 * MultiBootPatcher is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MultiBootPatcher is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MultiBootPatcher.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "mbdevice/device.h"

#define MB_DEVICE_ERROR_JSON            (-10)

#ifdef __cplusplus
extern "C" {
#endif

enum MbDeviceJsonErrorType
{
    // Use |line| and |column| fields
    MB_DEVICE_JSON_PARSE_ERROR,
    // Use |context|, |expected_type|, and |actual_type| fields
    MB_DEVICE_JSON_MISMATCHED_TYPE,
    // Use |context| fields
    MB_DEVICE_JSON_UNKNOWN_KEY,
    // Use |context] fields
    MB_DEVICE_JSON_UNKNOWN_VALUE,
};

struct MbDeviceJsonError
{
    enum MbDeviceJsonErrorType type;

    int line;
    int column;
    char context[100];
    char expected_type[20];
    char actual_type[20];
};

MB_EXPORT int mb_device_load_json(struct Device *device, const char *json,
                                  struct MbDeviceJsonError *error);

#ifdef __cplusplus
}
#endif
