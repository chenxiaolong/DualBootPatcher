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

#include "mbcommon/file.h"

#include <cassert>
#include <cerrno>
#include <cinttypes>
#include <cstdio>
#include <cstdlib>

#include "mbcommon/file_p.h"
#include "mbcommon/finally.h"

#define GET_PIMPL_OR_RETURN(RETVAL) \
    MB_PRIVATE(File); \
    do { \
        if (!priv) { \
            return RETVAL; \
        } \
    } while (0)

#define GET_PIMPL_OR_RETURN_ERROR() \
    GET_PIMPL_OR_RETURN(FileError::ObjectMoved)

#define ENSURE_STATE_OR_RETURN(STATES, RETVAL) \
    do { \
        if (!(priv->state & (STATES))) { \
            return RETVAL; \
        } \
    } while (0)

#define ENSURE_STATE_OR_RETURN_ERROR(STATES) \
    ENSURE_STATE_OR_RETURN(STATES, FileError::InvalidState)

// File documentation

/*!
 * \file mbcommon/file.h
 * \brief File abstraction API
 */

namespace mb
{

/*! \cond INTERNAL */
FilePrivate::FilePrivate() = default;

FilePrivate::~FilePrivate() = default;
/*! \endcond */

/*!
 * \class File
 *
 * \brief Utility class for reading and writing files.
 */

/*!
 * \var File::_priv_ptr
 *
 * \brief Pointer to pimpl object
 */

/*!
 * \brief Construct new File handle.
 */
File::File() : _priv_ptr(new FilePrivate())
{
    MB_PRIVATE(File);

    priv->state = FileState::New;
}

/*! \cond INTERNAL */
File::File(FilePrivate *priv_) : _priv_ptr(priv_)
{
    MB_PRIVATE(File);

    priv->state = FileState::New;
}
/*! \endcond */

/*!
 * \brief Destroy a File handle.
 *
 * If the handle has not been closed, it will be closed. Since this is the
 * destructor, it is not possible to get the result of the file close operation.
 * To get the result of the file close operation, call File::close() manually.
 */
File::~File()
{
    MB_PRIVATE(File);

    // We can't call a virtual function (on_close()) from the destructor, so we
    // must ensure that subclasses will call close().
    if (priv) {
        assert(priv->state == FileState::New);
    }
}

/*!
 * \brief Move construct new File handle.
 *
 * \p other will be left in a valid, but unspecified state. It can be brought
 * back to a useful state by assigning from a valid object. For example:
 *
 * \code{.cpp}
 * mb::StandardFile file1("foo.txt", mb::FileOpenMode::ReadOnly);
 * mb::StandardFile file2("bar.txt", mb::FileOpenMode::ReadOnly);
 * file1 = std::move(file2);
 * file2 = mb::StandardFile("baz.txt", mb::FileOpenMode::ReadOnly);
 * \endcode
 */
File::File(File &&other) noexcept : _priv_ptr(std::move(other._priv_ptr))
{
}

/*!
 * \brief Move assign a File handle
 *
 * The original file handle (`this`) will be closed and then \p rhs will be
 * moved into this object.
 *
 * \p other will be left in a valid, but unspecified state. It can be brought
 * back to a useful state by assigning from a valid object. For example:
 *
 * \code{.cpp}
 * mb::StandardFile file1("foo.txt", mb::FileOpenMode::ReadOnly);
 * mb::StandardFile file2("bar.txt", mb::FileOpenMode::ReadOnly);
 * file1 = std::move(file2);
 * file2 = mb::StandardFile("baz.txt", mb::FileOpenMode::ReadOnly);
 * \endcode
 */
File & File::operator=(File &&rhs) noexcept
{
    (void) close();

    _priv_ptr = std::move(rhs._priv_ptr);

    return *this;
}

/*!
 * \brief Open a File handle.
 *
 * Once the handle has been opened, the file operation functions, such as
 * File::read(), are available to use.
 *
 * \note This function should generally only be called by subclasses. Most
 *       subclasses will provide a variant of this function that can take
 *       parameters, such as a filename.
 *
 * \return Nothing if the file handle is successfully opened. Otherwise, the
 *         error code.
 */
oc::result<void> File::open()
{
    GET_PIMPL_OR_RETURN_ERROR();
    ENSURE_STATE_OR_RETURN_ERROR(FileState::New);

    auto ret = on_open();

    if (ret) {
        priv->state = FileState::Opened;
    } else {
        // If the file was not successfully opened, then close it
        (void) on_close();
    }

    return ret;
}

/*!
 * \brief Close a File handle.
 *
 * This function will close a File handle if it is open. Regardless of the
 * return value, the handle will be closed. The file handle can then be reused
 * for opening another file.
 *
 * \return
 *   * Nothing if no error was encountered when closing the handle.
 *   * An error code if the handle is opened and an error occurs while closing
 *     the file
 */
oc::result<void> File::close()
{
    GET_PIMPL_OR_RETURN_ERROR();

    auto reset_state = finally([&] {
        priv->state = FileState::New;
    });

    if (priv->state == FileState::New) {
        return oc::success();
    }

    return on_close();
}

/*!
 * \brief Read from a File handle.
 *
 * Example usage:
 *
 * \code{.cpp}
 * char buf[10240];
 *
 * while (true) {
 *     auto n = file.read(buf, sizeof(buf));
 *     if (!n) {
 *         printf("Failed to read file: %s\n", n.error().message().c_str());
 *         ...
 *     } else if (n == 0) {
 *         break;
 *     }
 *
 *     fwrite(buf, 1, n.value(), stdout);
 * }
 * \endcode
 *
 * \param[out] buf Buffer to read into
 * \param[in] size Buffer size
 *
 * \return Number of bytes read if some bytes were read or EOF was reached.
 *         Otherwise, the error code.
 */
oc::result<size_t> File::read(void *buf, size_t size)
{
    GET_PIMPL_OR_RETURN_ERROR();
    ENSURE_STATE_OR_RETURN_ERROR(FileState::Opened);

    return on_read(buf, size);
}

/*!
 * \brief Write to a File handle.
 *
 * Example usage:
 *
 * \code{.cpp}
 * auto n = file.write(buf, sizeof(buf));
 * if (!n) {
 *     printf("Failed to write file: %s\n", n.error().message().c_str());
 * } else {
 *     printf("Wrote %zu bytes\n", n.value());
 * }
 * \endcode
 *
 * \param buf Buffer to write from
 * \param size Buffer size
 *
 * \return Number of bytes that were written if some bytes were successfully
 *         written or EOF was reached. Otherwise, the error code.
 */
oc::result<size_t> File::write(const void *buf, size_t size)
{
    GET_PIMPL_OR_RETURN_ERROR();
    ENSURE_STATE_OR_RETURN_ERROR(FileState::Opened);

    return on_write(buf, size);
}

/*!
 * \brief Set file position of a File handle.
 *
 * \param offset File position offset
 * \param whence SEEK_SET, SEEK_CUR, or SEEK_END from `stdio.h`
 *
 * \return New file offset if the file position was successfully set. Otherwise,
 *         the error code.
 */
oc::result<uint64_t> File::seek(int64_t offset, int whence)
{
    GET_PIMPL_OR_RETURN_ERROR();
    ENSURE_STATE_OR_RETURN_ERROR(FileState::Opened);

    return on_seek(offset, whence);
}

/*!
 * \brief Truncate or extend file backed by a File handle.
 *
 * \note The file position is *not* changed after a successful call of this
 *       function. The size of the file may increase if the file position is
 *       larger than the truncated file size and File::write() is called.
 *
 * \param size New size of file
 *
 * \return Nothing if the file size was successfully changed. Otherwise, the
 *         error code.
 */
oc::result<void> File::truncate(uint64_t size)
{
    GET_PIMPL_OR_RETURN_ERROR();
    ENSURE_STATE_OR_RETURN_ERROR(FileState::Opened);

    return on_truncate(size);
}

/*!
 * \brief Check whether file is opened
 *
 * \return Whether file is opened
 */
bool File::is_open()
{
    GET_PIMPL_OR_RETURN(false);
    return priv->state == FileState::Opened;
}

/*!
 * \brief Check whether file is fatal state
 *
 * If the file is in the fatal state, the only valid operation is to call
 * close().
 *
 * \return Whether file is in fatal state
 */
bool File::is_fatal()
{
    GET_PIMPL_OR_RETURN(false);
    return priv->state == FileState::Fatal;
}

/*!
 * \brief Set whether file is fatal state
 *
 * This function only has an effect if the file is opened.
 *
 * If the file is in the fatal state, the only valid operation is to call
 * close().
 */
void File::set_fatal()
{
    GET_PIMPL_OR_RETURN();

    if (priv->state == FileState::Opened) {
        priv->state = FileState::Fatal;
    }
}

/*!
 * \brief File open callback
 *
 * Subclasses should override this method to implement the code needed to open
 * the file.
 *
 * The method should return:
 *
 *   * Nothing if the file was successfully opened
 *   * A specific error if an error occurred
 *
 * If this method is not overridden, it will simply return nothing.
 *
 * \return Always returns nothing
 */
oc::result<void> File::on_open()
{
    return oc::success();
}

/*!
 * \brief File close callback
 *
 * Subclasses should override this method to implement the code needed to close
 * the file and clean up any resources such that the file handle can be used to
 * open another file.
 *
 * This method will be called if:
 *
 *   * open() fails
 *   * close() is explicitly called, regardless if the file handle is in the
 *     opened or fatal state
 *   * the file handle is destroyed
 *
 * This method should return:
 *
 *   * Nothing if the file was successfully closed
 *   * A specific error if an error occurred
 *
 * \note Regardless of the return value, the file handle will be considered as
 *       closed and the file handle will allow opening another file.
 *
 * It is guaranteed that no other callbacks will be called, except for
 * on_open(), after this method returns.
 *
 * If this method is not overridden, it will simply return nothing.
 *
 * \return Always returns nothing
 */
oc::result<void> File::on_close()
{
    return oc::success();
}

/*!
 * \brief File read callback
 *
 * Subclasses should override this method to implement the code needed to read
 * from the file.
 *
 * This method should return:
 *
 *   * The number of bytes read if some bytes were successfully read or EOF was
 *     reached
 *   * std::errc::interrupted if the same operation should be reattempted
 *   * FileError::UnsupportedRead if the file does not support reading
 *   * A specific error for all other cases
 *
 * If this method is not overridden, it will simply return
 * FileError::UnsupportedRead.
 *
 * \param[out] buf Buffer to read into
 * \param[in] size Buffer size
 *
 * \return Always returns #FileError::UnsupportedRead
 */
oc::result<size_t> File::on_read(void *buf, size_t size)
{
    (void) buf;
    (void) size;

    return FileError::UnsupportedRead;
}

/*!
 * \brief File write callback
 *
 * Subclasses should override this method to implement the code needed to write
 * to the file.
 *
 * This method should return:
 *
 *   * The number of bytes written if some bytes were successfully written or
 *     EOF was reached
 *   * std::errc::interrupted if the same operation should be reattempted
 *   * FileError::UnsupportedWrite if the file does not support writing
 *   * A specific error for all other cases
 *
 * If this method is not overridden, it will simply return
 * FileError::UnsupportedWrite.
 *
 * \param buf Buffer to write from
 * \param size Buffer size
 *
 * \return Always returns #FileError::UnsupportedWrite
 */
oc::result<size_t> File::on_write(const void *buf, size_t size)
{
    (void) buf;
    (void) size;

    return FileError::UnsupportedWrite;
}

/*!
 * \brief File seek callback
 *
 * Subclasses should override this method to implement the code needed to
 * perform seeking on the file.
 *
 * This method should return:
 *
 *   * The new file position if the file position was successfully set
 *   * FileError::UnsupportedSeek if the file does not support seeking
 *   * A specific error for all other cases
 *
 * If this method is not overridden, it will simply return
 * FileError::UnsupportedSeek.
 *
 * \param offset File position offset
 * \param whence SEEK_SET, SEEK_CUR, or SEEK_END from `stdio.h`
 *
 * \return Always returns #FileError::UnsupportedSeek
 */
oc::result<uint64_t> File::on_seek(int64_t offset, int whence)
{
    (void) offset;
    (void) whence;

    return FileError::UnsupportedSeek;
}

/*!
 * \brief File truncate callback
 *
 * Subclasses should override this method to implement the code needed to
 * truncate or extend the file size.
 *
 * \note This callback must *not* change the file position.
 *
 * This method should return:
 *
 *   * Nothing if the file size was successfully changed
 *   * FileError::UnsupportedTruncate if the file does not support truncation
 *   * A specific error for all other cases
 *
 * If this method is not overridden, it will simply return
 * FileError::UnsupportedTruncate.
 *
 * \param size New size of file
 *
 * \return Always returns #FileError::UnsupportedTruncate
 */
oc::result<void> File::on_truncate(uint64_t size)
{
    (void) size;

    return FileError::UnsupportedTruncate;
}

}
