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
    device::Device device;
    std::string input_path;
    std::string output_path;
    std::string rom_id;
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
const std::string & FileInfo::input_path() const
{
    MB_PRIVATE(const FileInfo);
    return priv->input_path;
}

/*!
 * \brief Set file to be patched
 *
 * \param path File path
 */
void FileInfo::set_input_path(std::string path)
{
    MB_PRIVATE(FileInfo);
    priv->input_path = std::move(path);
}

const std::string & FileInfo::output_path() const
{
    MB_PRIVATE(const FileInfo);
    return priv->output_path;
}

void FileInfo::set_output_path(std::string path)
{
    MB_PRIVATE(FileInfo);
    priv->output_path = std::move(path);
}

/*!
 * \brief Target device
 *
 * \return Device
 */
const device::Device & FileInfo::device() const
{
    MB_PRIVATE(const FileInfo);
    return priv->device;
}

/*!
 * \brief Set target device
 *
 * \param device Target device
 */
void FileInfo::set_device(device::Device device)
{
    MB_PRIVATE(FileInfo);
    priv->device = std::move(device);
}

const std::string & FileInfo::rom_id() const
{
    MB_PRIVATE(const FileInfo);
    return priv->rom_id;
}

void FileInfo::set_rom_id(std::string id)
{
    MB_PRIVATE(FileInfo);
    priv->rom_id = std::move(id);
}

}
}
