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

#include "mbp/fileinfo.h"


namespace mbp
{

/*! \cond INTERNAL */
class FileInfo::Impl
{
public:
    Device *device;
    std::string inputPath;
    std::string outputPath;
    std::string romId;
};
/*! \endcond */


/*!
 * \class FileInfo
 * \brief Holds information about a file to be patched
 *
 * This is an extremely simple class that holds the following information about
 * a file to be patched:
 *
 * - File path
 * - Target Device
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
std::string FileInfo::inputPath() const
{
    return m_impl->inputPath;
}

/*!
 * \brief Set file to be patched
 *
 * \param path File path
 */
void FileInfo::setInputPath(std::string path)
{
    m_impl->inputPath = std::move(path);
}

std::string FileInfo::outputPath() const
{
    return m_impl->outputPath;
}

void FileInfo::setOutputPath(std::string path)
{
    m_impl->outputPath = std::move(path);
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

std::string FileInfo::romId() const
{
    return m_impl->romId;
}

void FileInfo::setRomId(std::string id)
{
    m_impl->romId = std::move(id);
}

}
