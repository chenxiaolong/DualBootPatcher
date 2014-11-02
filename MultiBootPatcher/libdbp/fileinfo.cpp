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

#include "fileinfo.h"
#include "patcherpaths.h"


class FileInfo::Impl
{
public:
    PatchInfo *patchinfo;
    Device *device;
    std::string filename;
    PartitionConfig *partconfig;
};


FileInfo::FileInfo() : m_impl(new Impl())
{
}

FileInfo::~FileInfo()
{
}

void FileInfo::setFilename(std::string path)
{
    m_impl->filename = std::move(path);
}

void FileInfo::setPatchInfo(PatchInfo * const info)
{
    m_impl->patchinfo = info;
}

void FileInfo::setDevice(Device * const device)
{
    m_impl->device = device;
}

void FileInfo::setPartConfig(PartitionConfig * const config)
{
    m_impl->partconfig = config;
}

std::string FileInfo::filename() const
{
    return m_impl->filename;
}

PatchInfo * FileInfo::patchInfo() const
{
    return m_impl->patchinfo;
}

Device * FileInfo::device() const
{
    return m_impl->device;
}

PartitionConfig * FileInfo::partConfig() const
{
    return m_impl->partconfig;
}
