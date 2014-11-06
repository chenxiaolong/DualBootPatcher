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


/*! \internal */
class FileInfo::Impl
{
public:
    PatchInfo *patchinfo;
    Device *device;
    std::string filename;
    PartitionConfig *partconfig;
};


/*!
 * \class FileInfo
 * \brief Holds information about a file to be patched
 *
 * This is an extremely simple class that holds the following information about
 * a file to be patched:
 *
 * - File path
 * - PatchInfo to use for patching
 * - Target Device
 * - Target PartitionConfig
 */


FileInfo::FileInfo() : m_impl(new Impl())
{
}

FileInfo::~FileInfo()
{
}

/*!
 * \brief File to be patched
 *
 * \return File path
 */
std::string FileInfo::filename() const
{
    return m_impl->filename;
}

/*!
 * \brief Set file to be patched
 *
 * \param path File path
 */
void FileInfo::setFilename(std::string path)
{
    m_impl->filename = std::move(path);
}

/*!
 * \brief PatchInfo used for patching the file
 *
 * \return PatchInfo object
 */
PatchInfo * FileInfo::patchInfo() const
{
    return m_impl->patchinfo;
}

/*!
 * \brief Set PatchInfo to be used for patching
 *
 * \param info PatchInfo object
 */
void FileInfo::setPatchInfo(PatchInfo * const info)
{
    m_impl->patchinfo = info;
}

/*!
 * \brief Target device
 *
 * \return Device
 */
Device * FileInfo::device() const
{
    return m_impl->device;
}

/*!
 * \brief Set target device
 *
 * \param device Target device
 */
void FileInfo::setDevice(Device * const device)
{
    m_impl->device = device;
}

/*!
 * \brief Target partition configuration
 *
 * \return Target PartitionConfig
 */
PartitionConfig * FileInfo::partConfig() const
{
    return m_impl->partconfig;
}

/*!
 * \brief Set target partition configuration
 *
 * \param config Target PartitionConfig
 */
void FileInfo::setPartConfig(PartitionConfig * const config)
{
    m_impl->partconfig = config;
}
