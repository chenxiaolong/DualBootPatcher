/*
 * Copyright (C) 2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbdevice/capi/device.h"

MB_BEGIN_C_DECLS

#define MB_DEVICE_JSON_PARSE_ERROR      (1u)
#define MB_DEVICE_JSON_MISMATCHED_TYPE  (2u)
#define MB_DEVICE_JSON_UNKNOWN_KEY      (3u)
#define MB_DEVICE_JSON_UNKNOWN_VALUE    (4u)

struct CJsonError;
typedef CJsonError CJsonError;

MB_EXPORT CJsonError * mb_device_json_error_new();

MB_EXPORT void mb_device_json_error_free(CJsonError *error);

MB_EXPORT uint16_t mb_device_json_error_type(const CJsonError *error);
MB_EXPORT int mb_device_json_error_line(const CJsonError *error);
MB_EXPORT int mb_device_json_error_column(const CJsonError *error);
MB_EXPORT char * mb_device_json_error_context(const CJsonError *error);
MB_EXPORT char * mb_device_json_error_expected_type(const CJsonError *error);
MB_EXPORT char * mb_device_json_error_actual_type(const CJsonError *error);

MB_EXPORT CDevice * mb_device_new_from_json(const char *json,
                                            CJsonError *error);

MB_EXPORT CDevice ** mb_device_new_list_from_json(const char *json,
                                                  CJsonError *error);

MB_EXPORT char * mb_device_to_json(const CDevice *device);

MB_END_C_DECLS
