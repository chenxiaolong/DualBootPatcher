/*
 * Copyright (C) 2016-2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbdevice/json.h"


namespace mb::device
{

bool device_from_json(const std::string &json, Device &device, JsonError &error)
{
    (void) json;
    (void) device;
    (void) error;

    return false;
}

bool device_list_from_json(const std::string &json,
                           std::vector<Device> &devices,
                           JsonError &error)
{
    (void) json;
    (void) devices;
    (void) error;

    return false;
}

bool device_to_json(const Device &device, std::string &json)
{
    (void) device;
    (void) json;

    return false;
}

}
