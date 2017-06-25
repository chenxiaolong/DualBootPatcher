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


namespace mbp
{

class MB_EXPORT FileInfo
{
public:
    explicit FileInfo();
    ~FileInfo();

    std::string inputPath() const;
    void setInputPath(std::string path);

    std::string outputPath() const;
    void setOutputPath(std::string path);

    Device * device() const;
    void setDevice(Device * const device);

    std::string romId() const;
    void setRomId(std::string id);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

}
