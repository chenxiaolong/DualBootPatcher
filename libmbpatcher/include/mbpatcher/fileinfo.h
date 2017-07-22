/*
 * Copyright (C) 2014-2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include <memory>

#include "mbcommon/common.h"
#include "mbdevice/device.h"


namespace mb
{
namespace patcher
{

class FileInfoPrivate;
class MB_EXPORT FileInfo
{
    MB_DECLARE_PRIVATE(FileInfo)

public:
    explicit FileInfo();
    ~FileInfo();

    const std::string & input_path() const;
    void set_input_path(std::string path);

    const std::string & output_path() const;
    void set_output_path(std::string path);

    const device::Device & device() const;
    void set_device(device::Device device);

    const std::string & rom_id() const;
    void set_rom_id(std::string id);

private:
    std::unique_ptr<FileInfoPrivate> _priv_ptr;
};

}
}
