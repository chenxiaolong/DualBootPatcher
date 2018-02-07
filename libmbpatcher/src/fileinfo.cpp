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

#include "mbpatcher/fileinfo.h"


namespace mb::patcher
{

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


/*!
 * \brief File to be patched
 *
 * \return File path
 */
const std::string & FileInfo::input_path() const
{
    return m_input_path;
}

/*!
 * \brief Set file to be patched
 *
 * \param path File path
 */
void FileInfo::set_input_path(std::string path)
{
    m_input_path = std::move(path);
}

const std::string & FileInfo::output_path() const
{
    return m_output_path;
}

void FileInfo::set_output_path(std::string path)
{
    m_output_path = std::move(path);
}

/*!
 * \brief Target device
 *
 * \return Device
 */
const device::Device & FileInfo::device() const
{
    return m_device;
}

/*!
 * \brief Set target device
 *
 * \param device Target device
 */
void FileInfo::set_device(device::Device device)
{
    m_device = std::move(device);
}

const std::string & FileInfo::rom_id() const
{
    return m_rom_id;
}

void FileInfo::set_rom_id(std::string id)
{
    m_rom_id = std::move(id);
}

}
