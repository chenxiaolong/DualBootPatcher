/*
 * Copyright (C) 2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbdevice/device.h"

namespace mb::device
{

enum class JsonErrorType : uint16_t
{
    // Use |offset| and |message| fields
    ParseError = 1,
    // Use |schema_uri|, |schema_keyword|, and |document_uri| fields
    SchemaValidationFailure,
};

struct JsonError
{
    JsonErrorType type;

    size_t offset;
    std::string message;

    std::string schema_uri;
    std::string schema_keyword;
    std::string document_uri;
};

MB_EXPORT bool device_from_json(const std::string &json, Device &device,
                                JsonError &error);

MB_EXPORT bool device_list_from_json(const std::string &json,
                                     std::vector<Device> &devices,
                                     JsonError &error);

MB_EXPORT bool device_to_json(const Device &device, std::string &json);

}
