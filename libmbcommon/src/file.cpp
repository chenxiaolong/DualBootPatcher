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
#include "mbcommon/string.h"

#define GET_PIMPL_OR_RETURN(RETVAL) \
    MB_PRIVATE(File); \
    do { \
        if (!priv) { \
            return RETVAL; \
        } \
    } while (0)

#define ENSURE_STATE_OR_RETURN(STATES, RETVAL) \
    do { \
        if (!(priv->state & (STATES))) { \
            set_error(make_error_code(FileError::InvalidState), \
                      "%s: Invalid state: "\
                      "expected 0x%" PRIx16 ", actual: 0x%" PRIx16, \
                      __func__, static_cast<uint16_t>(STATES), \
                      static_cast<uint16_t>(priv->state)); \
            return RETVAL; \
        } \
    } while (0)

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

    priv->state = FileState::NEW;
}

/*! \cond INTERNAL */
File::File(FilePrivate *priv_) : _priv_ptr(priv_)
{
    MB_PRIVATE(File);

    priv->state = FileState::NEW;
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
        assert(priv->state == FileState::NEW);
    }
}

/*!
 * \brief Move construct new File handle.
 *
 * \p other will be left in a valid, but unspecified state. It can be brought
 * back to a useful state by assigning from a valid object. For example:
 *
 * \code{.cpp}
 * mb::StandardFile file1("foo.txt", mb::FileOpenMode::READ_ONLY);
 * mb::StandardFile file2("bar.txt", mb::FileOpenMode::READ_ONLY);
 * file1 = std::move(file2);
 * file2 = mb::StandardFile("baz.txt", mb::FileOpenMode::READ_ONLY);
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
 * mb::StandardFile file1("foo.txt", mb::FileOpenMode::READ_ONLY);
 * mb::StandardFile file2("bar.txt", mb::FileOpenMode::READ_ONLY);
 * file1 = std::move(file2);
 * file2 = mb::StandardFile("baz.txt", mb::FileOpenMode::READ_ONLY);
 * \endcode
 */
File & File::operator=(File &&rhs) noexcept
{
    close();

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
 * \return Whether the file handle is successfully opened
 */
bool File::open()
{
    GET_PIMPL_OR_RETURN(false);
    ENSURE_STATE_OR_RETURN(FileState::NEW, false);

    auto ret = on_open();

    if (ret) {
        priv->state = FileState::OPENED;
    } else {
        // If the file was not successfully opened, then close it
        on_close();
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
 *   * True if no error was encountered when closing the handle.
 *   * False if the handle is opened and an error occurs while closing the file
 */
bool File::close()
{
    GET_PIMPL_OR_RETURN(false);

    auto ret = true;

    // Avoid double-closing or closing nothing
    if (priv->state != FileState::NEW) {
        ret = on_close();
    }

    priv->state = FileState::NEW;

    return ret;
}

/*!
 * \brief Read from a File handle.
 *
 * Example usage:
 *
 * \code{.cpp}
 * char buf[10240];
 * int ret;
 * size_t n;
 *
 * while ((ret = file.read(buf, sizeof(buf), &n)) && n >= 0) {
 *     fwrite(buf, 1, n, stdout);
 * }
 *
 * if (!ret) {
 *     printf("Failed to read file: %s\n", file.error_string(file).c_str());
 * }
 * \endcode
 *
 * \param[out] buf Buffer to read into
 * \param[in] size Buffer size
 * \param[out] bytes_read Output number of bytes that were read. 0 indicates end
 *                        of file.
 *
 * \return Whether some bytes were read or EOF was reached
 */
bool File::read(void *buf, size_t size, size_t &bytes_read)
{
    GET_PIMPL_OR_RETURN(false);
    ENSURE_STATE_OR_RETURN(FileState::OPENED, false);

    return on_read(buf, size, bytes_read);
}

/*!
 * \brief Write to a File handle.
 *
 * Example usage:
 *
 * \code{.cpp}
 * size_t n;
 *
 * if (!file.write(file, buf, sizeof(buf), &bytesWritten)) {
 *     printf("Failed to write file: %s\n", file.error_string().c_str());
 * }
 * \endcode
 *
 * \param[in] buf Buffer to write from
 * \param[in] size Buffer size
 * \param[out] bytes_written Output number of bytes that were written.
 *
 * \return Whether some bytes were successfully written
 */
bool File::write(const void *buf, size_t size, size_t &bytes_written)
{
    GET_PIMPL_OR_RETURN(false);
    ENSURE_STATE_OR_RETURN(FileState::OPENED, false);

    return on_write(buf, size, bytes_written);
}

/*!
 * \brief Set file position of a File handle.
 *
 * \param[in] offset File position offset
 * \param[in] whence SEEK_SET, SEEK_CUR, or SEEK_END from `stdio.h`
 * \param[out] new_offset Output new file offset. This parameter can be NULL.
 *
 * \return Whether the file position was successfully set
 */
bool File::seek(int64_t offset, int whence, uint64_t *new_offset)
{
    GET_PIMPL_OR_RETURN(false);
    ENSURE_STATE_OR_RETURN(FileState::OPENED, false);

    uint64_t new_offset_temp;

    auto ret = on_seek(offset, whence, new_offset_temp);

    if (ret) {
        if (new_offset) {
            *new_offset = new_offset_temp;
        }
    }

    return ret;
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
 * \return Whether the file size was successfully changed
 */
bool File::truncate(uint64_t size)
{
    GET_PIMPL_OR_RETURN(false);
    ENSURE_STATE_OR_RETURN(FileState::OPENED, false);

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
    return priv->state == FileState::OPENED;
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
    return priv->state == FileState::FATAL;
}

/*!
 * \brief Set whether file is fatal state
 *
 * This function can only be called if the file is opened.
 *
 * If the file is in the fatal state, the only valid operation is to call
 * close().
 *
 * \return Whether the fatal state was successfully set
 */
bool File::set_fatal(bool fatal)
{
    GET_PIMPL_OR_RETURN(false);
    ENSURE_STATE_OR_RETURN(FileState::OPENED | FileState::FATAL, false);

    priv->state = fatal ? FileState::FATAL : FileState::OPENED;
    return true;
}

/*!
 * \brief Get error code for a failed operation.
 *
 * \note The return value is undefined if an operation did not fail.
 *
 * \return Error code for failed operation. Test against FileError or std::errc.
 */
std::error_code File::error()
{
    GET_PIMPL_OR_RETURN({});

    return priv->error_code;
}

/*!
 * \brief Get error string for a failed operation.
 *
 * \note The return value is undefined if an operation did not fail.
 *
 * \return Error string for failed operation. The string contents may be
 *         undefined.
 */
std::string File::error_string()
{
    GET_PIMPL_OR_RETURN({});

    return priv->error_string;
}

/*!
 * \brief Set error string for a failed operation.
 *
 * \sa File::set_error_v()
 *
 * \param ec Error code
 * \param fmt `printf()`-style format string
 * \param ... `printf()`-style format arguments
 *
 * \return Whether the error was successfully set
 */
bool File::set_error(std::error_code ec, const char *fmt, ...)
{
    bool ret;
    va_list ap;

    va_start(ap, fmt);
    ret = set_error_v(ec, fmt, ap);
    va_end(ap);

    return ret;
}

/*!
 * \brief Set error string for a failed operation.
 *
 * \sa File::set_error()
 *
 * \param ec Error code
 * \param fmt `printf()`-style format string
 * \param ap `printf()`-style format arguments as a va_list
 *
 * \return Whether the error was successfully set
 */
bool File::set_error_v(std::error_code ec, const char *fmt, va_list ap)
{
    GET_PIMPL_OR_RETURN(false);

    priv->error_code = ec;

    if (!format_v(priv->error_string, fmt, ap)) {
        return false;
    }

    priv->error_string += ": ";
    priv->error_string += ec.message();

    return true;
}

/*!
 * \brief File open callback
 *
 * Subclasses should override this method to implement the code needed to open
 * the file.
 *
 * The method should return:
 *
 *   * True if the file was successfully opened
 *   * False and set specific error if an error occurred
 *
 * If this method is not overridden, it will simply return true.
 *
 * \return Always returne true
 */
bool File::on_open()
{
    return true;
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
 *   * True if the file was successfully closed
 *   * False and set specific error if an error occurred
 *
 * \note Regardless of the return value, the file handle will be considered as
 *       closed and the file handle will allow opening another file.
 *
 * It is guaranteed that no other callbacks will be called, except for
 * on_open(), after this method returns.
 *
 * If this method is not overridden, it will simply return true.
 *
 * \return Always returns true
 */
bool File::on_close()
{
    return true;
}

/*!
 * \brief File read callback
 *
 * Subclasses should override this method to implement the code needed to read
 * from the file.
 *
 * This method should return:
 *
 *   * True if some bytes were read or EOF was reached
 *   * False and set error to std::errc::interrupted if the same operation
 *     should be reattempted
 *   * False and set error to FileError::UnsupportedRead if the file does not
 *     support reading
 *   * False and set specific error for all other cases
 *
 * If this method is not overridden, it will simply return false and set the
 * error to FileError::UnsupportedRead.
 *
 * \param[out] buf Buffer to read into
 * \param[in] size Buffer size
 * \param[out] bytes_read Output number of bytes that were read. 0 indicates end
 *                        of file. This parameter is guaranteed to be non-NULL.
 *
 * \return Always returns false and sets the error to
 *         #FileError::UnsupportedRead
 */
bool File::on_read(void *buf, size_t size, size_t &bytes_read)
{
    (void) buf;
    (void) size;
    (void) bytes_read;

    set_error(make_error_code(FileError::UnsupportedRead),
              "%s: Read callback not supported", __func__);
    return false;
}

/*!
 * \brief File write callback
 *
 * Subclasses should override this method to implement the code needed to write
 * to the file.
 *
 * This method should return:
 *
 *   * True if some bytes were written
 *   * False and set error to std::errc::interrupted if the same operation
 *     should be reattempted
 *   * False and set error to FileError::UnsupportedWrite if the file does not
 *     support writing
 *   * False and set specific error for all other cases
 *
 * If this method is not overridden, it will simply return false and set the
 * error to FileError::UnsupportedWrite.
 *
 * \param[in] buf Buffer to write from
 * \param[in] size Buffer size
 * \param[out] bytes_written Output number of bytes that were written. This
 *                           parameter is guaranteed to be non-NULL.
 *
 * \return Always returns false and sets the error to
 *         #FileError::UnsupportedWrite
 */
bool File::on_write(const void *buf, size_t size, size_t &bytes_written)
{
    (void) buf;
    (void) size;
    (void) bytes_written;

    set_error(make_error_code(FileError::UnsupportedWrite),
              "%s: Write callback not supported", __func__);
    return false;
}

/*!
 * \brief File seek callback
 *
 * Subclasses should override this method to implement the code needed to
 * perform seeking on the file.
 *
 * This method should return:
 *
 *   * True if the file position was successfully set
 *   * False and set error to FileError::UnsupportedSeek if the file does not
 *     support seeking
 *   * False and set specific error for all other cases
 *
 * If this method is not overridden, it will simply return false and set the
 * error to FileError::UnsupportedSeek.
 *
 * \param[in] offset File position offset
 * \param[in] whence SEEK_SET, SEEK_CUR, or SEEK_END from `stdio.h`
 * \param[out] new_offset Output new file offset
 *
 * \return Always returns false and sets the error to
 *         #FileError::UnsupportedSeek
 */
bool File::on_seek(int64_t offset, int whence, uint64_t &new_offset)
{
    (void) offset;
    (void) whence;
    (void) new_offset;

    set_error(make_error_code(FileError::UnsupportedSeek),
              "%s: Seek callback not supported", __func__);
    return false;
}

/*!
 * \brief File truncate callback
 *
 * Subclasses should override this method to implement the code needed to
 * truncate to extend the file size.
 *
 * \note This callback must *not* change the file position.
 *
 * This method should return:
 *
 *   * True if the file size was successfully changed
 *   * False and set error to FileError::UnsupportedTruncate if the file does
 *     not support truncation
 *   * False and set specific error for all other cases
 *
 * If this method is not overridden, it will simply return false and set the
 * error to FileError::UnsupportedTruncate.
 *
 * \param size New size of file
 *
 * \return Always returns false and sets the error to
 *         #FileError::UnsupportedTruncate
 */
bool File::on_truncate(uint64_t size)
{
    (void) size;

    set_error(make_error_code(FileError::UnsupportedTruncate),
              "%s: Truncate callback not supported", __func__);
    return false;
}

}
