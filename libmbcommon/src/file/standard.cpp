/*
 * Copyright (C) 2016-2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbcommon/file/standard.h"

/*!
 * \file mbcommon/file/standard.h
 * \brief Open file with filename
 */

namespace mb
{

/*!
 * \def STANDARD_FILE_IMPL
 *
 * \brief Macro that defines the best base class for StandardFile depending on
 *        the platform.
 */

/*!
 * \class StandardFile
 *
 * \brief Open file from filename.
 *
 * This class is guaranteed to support large files on all supported platforms.
 *
 *   * On Windows systems, the mbcommon/file/win32.h API is used
 *   * On Android systems, the mbcommon/file/fd.h API is used
 *   * On other Unix-like systems, the mbcommon/file/posix.h API is used
 */

/*!
 * \brief Construct unbound StandardFile.
 *
 * The File handle will not be bound to any file. One of the open functions will
 * need to be called to open a file.
 */
StandardFile::StandardFile() = default;

/*!
 * \brief Open File handle from a multi-byte filename.
 *
 * Construct the file handle and open the file. Use is_open() to check if the
 * file was successfully opened.
 *
 * \sa open(const std::string &, FileOpenMode)
 *
 * \param filename MBS filename
 * \param mode Open mode (\ref FileOpenMode)
 */
StandardFile::StandardFile(const std::string &filename, FileOpenMode mode)
    : m_file(filename, mode)
{
}

/*!
 * \brief Open File handle from a wide-character filename.
 *
 * Construct the file handle and open the file. Use is_open() to check if the
 * file was successfully opened.
 *
 * \sa open(const std::wstring &, FileOpenMode)
 *
 * \param filename WCS filename
 * \param mode Open mode (\ref FileOpenMode)
 */
StandardFile::StandardFile(const std::wstring &filename, FileOpenMode mode)
    : m_file(filename, mode)
{
}

StandardFile::~StandardFile() = default;

/*!
 * \brief Open from a multi-byte filename.
 *
 * \param filename MBS filename
 * \param mode Open mode (\ref FileOpenMode)
 *
 * \return Nothing if the file is successfully opened. Otherwise, the error
 *         code.
 */
oc::result<void>
StandardFile::open(const std::string &filename, FileOpenMode mode)
{
    return m_file.open(filename, mode);
}

/*!
 * \brief Open from a wide-character filename.
 *
 * \param filename WCS filename
 * \param mode Open mode (\ref FileOpenMode)
 *
 * \return Nothing if the file is successfully opened. Otherwise, the error
 *         code.
 */
oc::result<void>
StandardFile::open(const std::wstring &filename, FileOpenMode mode)
{
    return m_file.open(filename, mode);
}

oc::result<void> StandardFile::close()
{
    return m_file.close();
}

oc::result<size_t> StandardFile::read(void *buf, size_t size)
{
    return m_file.read(buf, size);
}

oc::result<size_t> StandardFile::write(const void *buf, size_t size)
{
    return m_file.write(buf, size);
}

oc::result<uint64_t> StandardFile::seek(int64_t offset, int whence)
{
    return m_file.seek(offset, whence);
}

oc::result<void> StandardFile::truncate(uint64_t size)
{
    return m_file.truncate(size);
}

bool StandardFile::is_open()
{
    return m_file.is_open();
}

}
