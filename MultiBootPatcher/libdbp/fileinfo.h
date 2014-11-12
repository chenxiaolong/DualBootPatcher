/*
 * Copyright (C) 2014  Xiao-Long Chen <chenxiaolong@cxl.epac.to>
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

#ifndef FILEINFO_H
#define FILEINFO_H

#include <memory>

#include "libdbp_global.h"

#include "device.h"
#include "patchinfo.h"
#include "partitionconfig.h"


class MBP_EXPORT FileInfo
{
public:
    explicit FileInfo();
    ~FileInfo();

    std::string filename() const;
    void setFilename(std::string path);

    PatchInfo * patchInfo() const;
    void setPatchInfo(PatchInfo * const info);

    Device * device() const;
    void setDevice(Device * const device);

    PartitionConfig * partConfig() const;
    void setPartConfig(PartitionConfig * const config);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

#endif // FILEINFO_H
