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

#include "mbdevice/capi/json.h"

#include <cassert>
#include <cstdlib>
#include <cstring>

#include "mbcommon/capi/util.h"
#include "mbdevice/json.h"

#define D_CAST(x) \
    assert(x != nullptr); \
    auto *d = reinterpret_cast<Device *>(x);
#define D_CCAST(x) \
    assert(x != nullptr); \
    auto const *d = reinterpret_cast<const Device *>(x);
#define JE_CAST(x) \
    assert(x != nullptr); \
    auto *je = reinterpret_cast<JsonError *>(x);
#define JE_CCAST(x) \
    assert(x != nullptr); \
    auto const *je = reinterpret_cast<const JsonError *>(x);

using namespace mb;
using namespace mb::device;

MB_BEGIN_C_DECLS

CJsonError * mb_device_json_error_new()
{
    return reinterpret_cast<CJsonError *>(new JsonError());
}

void mb_device_json_error_free(CJsonError *error)
{
    delete reinterpret_cast<JsonError *>(error);
}

uint16_t mb_device_json_error_type(const CJsonError *error)
{
    JE_CCAST(error);
    return static_cast<std::underlying_type_t<JsonErrorType>>(je->type);
}

size_t mb_device_json_error_offset(const CJsonError *error)
{
    JE_CCAST(error);
    return je->offset;
}

char * mb_device_json_error_message(const CJsonError *error)
{
    JE_CCAST(error);
    return mb::capi_str_to_cstr(je->message);
}

char * mb_device_json_error_schema_uri(const CJsonError *error)
{
    JE_CCAST(error);
    return mb::capi_str_to_cstr(je->schema_uri);
}

char * mb_device_json_error_schema_keyword(const CJsonError *error)
{
    JE_CCAST(error);
    return mb::capi_str_to_cstr(je->schema_keyword);
}

char * mb_device_json_error_document_uri(const CJsonError *error)
{
    JE_CCAST(error);
    return mb::capi_str_to_cstr(je->document_uri);
}

CDevice * mb_device_new_from_json(const char *json, CJsonError *error)
{
    JE_CAST(error);

    Device *d = new Device();

    if (!device_from_json(json, *d, *je)) {
        delete d;
        return nullptr;
    }

    return reinterpret_cast<CDevice *>(d);
}

CDevice ** mb_device_new_list_from_json(const char *json, CJsonError *error)
{
    JE_CAST(error);

    std::vector<Device> devices;

    if (!device_list_from_json(json, devices, *je)) {
        return nullptr;
    }

    if (devices.size() == SIZE_MAX
            || (devices.size() + 1) > SIZE_MAX / sizeof(CDevice *)) {
        return nullptr;
    }

    CDevice **array = static_cast<CDevice **>(
            malloc(sizeof(CDevice *) * (devices.size() + 1)));
    if (!array) {
        return nullptr;
    }

    // NOTE: Not exception safe, but we don't compile with exceptions enabled
    for (size_t i = 0; i < devices.size(); ++i) {
        array[i] = reinterpret_cast<CDevice *>(
                new Device(std::move(devices[i])));
    }
    array[devices.size()] = nullptr;

    return array;
}

char * mb_device_to_json(const CDevice *device)
{
    D_CCAST(device);

    std::string result;

    if (!device_to_json(*d, result)) {
        return nullptr;
    }

    return capi_str_to_cstr(result);
}

MB_END_C_DECLS
