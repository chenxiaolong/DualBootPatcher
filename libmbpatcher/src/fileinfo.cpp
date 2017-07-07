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

#include "mbpatcher/fileinfo.h"


namespace mb
{
namespace patcher
{

/*! \cond INTERNAL */
class FileInfoPrivate
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


FileInfo::FileInfo() : _priv_ptr(new FileInfoPrivate())
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
    MB_PRIVATE(const FileInfo);
    return priv->inputPath;
}

/*!
 * \brief Set file to be patched
 *
 * \param path File path
 */
void FileInfo::setInputPath(std::string path)
{
    MB_PRIVATE(FileInfo);
    priv->inputPath = std::move(path);
}

std::string FileInfo::outputPath() const
{
    MB_PRIVATE(const FileInfo);
    return priv->outputPath;
}

void FileInfo::setOutputPath(std::string path)
{
    MB_PRIVATE(FileInfo);
    priv->outputPath = std::move(path);
}

/*!
 * \brief Target device
 *
 * \return Device
 */
Device * FileInfo::device() const
{
    MB_PRIVATE(const FileInfo);
    return priv->device;
}

/*!
 * \brief Set target device
 *
 * \param device Target device
 */
void FileInfo::setDevice(Device * const device)
{
    MB_PRIVATE(FileInfo);
    priv->device = device;
}

std::string FileInfo::romId() const
{
    MB_PRIVATE(const FileInfo);
    return priv->romId;
}

void FileInfo::setRomId(std::string id)
{
    MB_PRIVATE(FileInfo);
    priv->romId = std::move(id);
}

}
}
