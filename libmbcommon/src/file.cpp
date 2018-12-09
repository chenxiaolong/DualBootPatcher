/*
 * Copyright (C) 2016-2018  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbcommon/file.h"

// File documentation

/*!
 * \file mbcommon/file.h
 * \brief File abstraction API
 */

namespace mb
{

/*!
 * \class File
 *
 * \brief Interface for file-like objects.
 *
 * This class is a basic interface for file-like objects implementing some or
 * all of the read, write, seek, truncate, or close operations. This class does
 * not actually contain any logic.
 *
 * Subclasses should override the pure-virtual functions and implement them
 * according to the functions' documentation.
 */

/*!
 * \brief Construct new File handle.
 */
File::File() = default;

/*!
 * \brief Destroy a File handle.
 *
 * If the handle has not been closed, it will be closed. Since this is the
 * destructor, it is not possible to get the result of the file close operation.
 * To get the result of the file close operation, call File::close() manually.
 */
File::~File() = default;

/*!
 * \fn File::close()
 *
 * \brief Close a File handle.
 *
 * This function will close a File handle if it is open. Regardless of the
 * return value, the handle will be closed. The file handle can then be reused
 * for opening another file.
 *
 * \return
 *   * Nothing if no error is encountered when closing the handle
 *   * An error code if the handle is opened and an error occurs while closing
 *     the file
 */

/*!
 * \fn File::read()
 *
 * \brief Read from a File handle.
 *
 * Example usage:
 *
 * \code{.cpp}
 * char buf[10240];
 *
 * while (true) {
 *     if (auto n = file.read(buf, sizeof(buf))) {
 *         if (n.value() == 0) {
 *             break;
 *         } else {
 *             fwrite(buf, 1, n.value(), stdout);
 *         }
 *     } else {
 *         printf("Failed to read file: %s\n", n.error().message().c_str());
 *         ...
 *     }
 * }
 * \endcode
 *
 * \param[out] buf Buffer to read into
 * \param[in] size Buffer size
 *
 * \return
 *   * Number of bytes read if some bytes are successfully read or EOF is
 *     reached
 *   * std::errc::interrupted if the same operation should be reattempted
 *   * FileError::UnsupportedRead if the file does not support reading
 *   * Otherwise, a specific error code
 */

/*!
 * \fn File::write()
 *
 * \brief Write to a File handle.
 *
 * Example usage:
 *
 * \code{.cpp}
 * if (auto n = file.write(buf, sizeof(buf))) {
 *     printf("Wrote %zu bytes\n", n.value());
 * } else {
 *     printf("Failed to write file: %s\n", n.error().message().c_str());
 * }
 * \endcode
 *
 * \param buf Buffer to write from
 * \param size Buffer size
 *
 * \return
 *   * Number of bytes written if some bytes are successfully written or EOF is
 *     reached
 *   * std::errc::interrupted if the same operation should be reattempted
 *   * FileError::UnsupportedWrite if the file does not support writing
 *   * Otherwise, a specific error code
 */

/*!
 * \fn File::seek()
 *
 * \brief Set file position of a File handle.
 *
 * \param offset File position offset
 * \param whence SEEK_SET, SEEK_CUR, or SEEK_END from `stdio.h`
 *
 * \return
 *   * New file offset if the file position is successfully set
 *   * FileError::UnsupportedSeek is the file does not support seeking
 *   * Otherwise, a specific error code
 */

/*!
 * \fn File::truncate()
 *
 * \brief Truncate or extend file backed by a File handle.
 *
 * \note The file position is *not* changed after a successful call of this
 *       function. The size of the file may increase if the file position is
 *       larger than the truncated file size and File::write() is called.
 *
 * \param size New size of file
 *
 * \return
 *   * Nothing if the file size is successfully changed
 *   * FileError::UnsupportedTruncate if the file does not support truncation
 *   * Otherwise, a specific error code
 */

/*!
 * \brief Check whether file is opened
 *
 * \return Whether file is opened
 */

}
