/*
 * Copyright (C) 2014-2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbcommon/common.h"
#include "mbdevice/device.h"


namespace mb::patcher
{

class MB_EXPORT FileInfo
{
public:
    const std::string & input_path() const;
    void set_input_path(std::string path);

    const std::string & output_path() const;
    void set_output_path(std::string path);

    const device::Device & device() const;
    void set_device(device::Device device);

    const std::string & rom_id() const;
    void set_rom_id(std::string id);

private:
    device::Device m_device;
    std::string m_input_path;
    std::string m_output_path;
    std::string m_rom_id;
};

}
