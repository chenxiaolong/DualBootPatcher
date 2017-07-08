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

#define ENSURE_STATE(STATES) \
    do { \
        if (!(priv->state & (STATES))) { \
            set_error(FileError::PROGRAMMER_ERROR, \
                      "%s: Invalid state: "\
                      "expected 0x%hx, actual: 0x%hx", \
                      __func__, (STATES), priv->state); \
            priv->state = FileState::FATAL; \
            return FileStatus::FATAL; \
        } \
    } while (0)

// File documentation

/*!
 * \file mbcommon/file.h
 * \brief File abstraction API
 */

namespace mb
{

// Return values documentation

/*!
 * \enum FileStatus
 *
 * \brief Possible return values for functions.
 */

/*!
 * \var FileStatus::OK
 *
 * \brief Success error code
 *
 * Success error code.
 */

/*!
 * \var FileStatus::RETRY
 *
 * \brief Reattempt operation
 *
 * The operation should be reattempted.
 */

/*!
 * \var FileStatus::WARN
 *
 * \brief Warning
 *
 * The operation raised a warning. The File handle can still be used although
 * the functionality may be degraded.
 */

/*!
 * \var FileStatus::FAILED
 *
 * \brief Non-fatal error
 *
 * The operation failed non-fatally. The File handle can still be used for
 * further operations.
 */

/*!
 * \var FileStatus::FATAL
 *
 * \brief Fatal error
 *
 * The operation failed fatally. The File handle can no longer be used for
 * further operations.
 */

/*!
 * \var FileStatus::UNSUPPORTED
 *
 * \brief Operation not supported
 *
 * The operation is not supported.
 */

// Error codes documentation

/*!
 * \namespace FileError
 *
 * \brief Possible error codes.
 */

/*!
 * \var FileError::NONE
 *
 * \brief No error
 */

/*!
 * \var FileError::INVALID_ARGUMENT
 *
 * \brief An invalid argument was provided
 */

/*!
 * \var FileError::UNSUPPORTED
 *
 * \brief The operation is not supported
 */

/*!
 * \var FileError::PROGRAMMER_ERROR
 *
 * \brief The function were called in an invalid state
 */

/*!
 * \var FileError::INTERNAL_ERROR
 *
 * \brief Internal error in the library
 */

/*! \cond INTERNAL */
FilePrivate::FilePrivate()
{
}

FilePrivate::~FilePrivate()
{
}
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
File::File(File &&other) : _priv_ptr(std::move(other._priv_ptr))
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
File & File::operator=(File &&rhs)
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
 * \return
 *   * #FileStatus::OK if there is no file open handler or if the file open
 *     handle succeeds
 *   * \<= #FileStatus::WARN if an error occurs
 */
FileStatus File::open()
{
    GET_PIMPL_OR_RETURN(FileStatus::FATAL);
    ENSURE_STATE(FileState::NEW);

    auto ret = on_open();

    if (ret == FileStatus::OK) {
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
 *   * #FileStatus::OK if no error was encountered when closing the handle.
 *   * \<= #FileStatus::WARN if the handle is opened and an error occurs while
 *     closing the file
 */
FileStatus File::close()
{
    GET_PIMPL_OR_RETURN(FileStatus::FATAL);

    auto ret = FileStatus::OK;

    // Avoid double-closing or closing nothing
    if (!(priv->state & FileState::NEW)) {
        ret = on_close();

        // Don't change state to FileState::FATAL if FileStatus::FATAL is
        // returned. Otherwise, we risk double-closing the file.
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
 * while ((ret = file.read(buf, sizeof(buf), &n)) == FileStatus::OK
 *         && n >= 0) {
 *     fwrite(buf, 1, n, stdout);
 * }
 *
 * if (ret != FileStatus::OK) {
 *     printf("Failed to read file: %s\n", file.error_string(file).c_str());
 * }
 * \endcode
 *
 * \param[out] buf Buffer to read into
 * \param[in] size Buffer size
 * \param[out] bytes_read Output number of bytes that were read. 0 indicates end
 *                        of file. This parameter cannot be NULL.
 *
 * \return
 *   * #FileStatus::OK if some bytes were read or EOF is reached
 *   * #FileStatus::RETRY if the same operation should be reattempted
 *   * #FileStatus::UNSUPPORTED if the handle source does not support reading
 *   * \<= #FileStatus::WARN if an error occurs
 */
FileStatus File::read(void *buf, size_t size, size_t *bytes_read)
{
    GET_PIMPL_OR_RETURN(FileStatus::FATAL);
    ENSURE_STATE(FileState::OPENED);

    auto ret = FileStatus::UNSUPPORTED;

    if (!bytes_read) {
        set_error(FileError::PROGRAMMER_ERROR,
                  "%s: bytes_read is NULL", __func__);
        ret = FileStatus::FATAL;
    } else {
        ret = on_read(buf, size, bytes_read);
    }
    if (ret <= FileStatus::FATAL) {
        priv->state = FileState::FATAL;
    }

    return ret;
}

/*!
 * \brief Write to a File handle.
 *
 * Example usage:
 *
 * \code{.cpp}
 * size_t n;
 *
 * if (file.write(file, buf, sizeof(buf), &bytesWritten)
 *         != FileStatus::OK) {
 *     printf("Failed to write file: %s\n", file.error_string().c_str());
 * }
 * \endcode
 *
 * \param[in] buf Buffer to write from
 * \param[in] size Buffer size
 * \param[out] bytes_written Output number of bytes that were written. This
 *                           parameter cannot be NULL.
 *
 * \return
 *   * #FileStatus::OK if some bytes were written
 *   * #FileStatus::RETRY if the same operation should be reattempted
 *   * #FileStatus::UNSUPPORTED if the handle source does not support writing
 *   * \<= #FileStatus::WARN if an error occurs
 */
FileStatus File::write(const void *buf, size_t size, size_t *bytes_written)
{
    GET_PIMPL_OR_RETURN(FileStatus::FATAL);
    ENSURE_STATE(FileState::OPENED);

    auto ret = FileStatus::UNSUPPORTED;

    if (!bytes_written) {
        set_error(FileError::PROGRAMMER_ERROR,
                  "%s: bytes_written is NULL", __func__);
        ret = FileStatus::FATAL;
    } else {
        ret = on_write(buf, size, bytes_written);
    }
    if (ret <= FileStatus::FATAL) {
        priv->state = FileState::FATAL;
    }

    return ret;
}

/*!
 * \brief Set file position of a File handle.
 *
 * \param[in] offset File position offset
 * \param[in] whence SEEK_SET, SEEK_CUR, or SEEK_END from `stdio.h`
 * \param[out] new_offset Output new file offset. This parameter can be NULL.
 *
 * \return
 *   * #FileStatus::OK if the file position was successfully set
 *   * #FileStatus::UNSUPPORTED if the handle source does not support seeking
 *   * \<= #FileStatus::WARN if an error occurs
 */
FileStatus File::seek(int64_t offset, int whence, uint64_t *new_offset)
{
    GET_PIMPL_OR_RETURN(FileStatus::FATAL);
    ENSURE_STATE(FileState::OPENED);

    uint64_t new_offset_temp;

    auto ret = on_seek(offset, whence, &new_offset_temp);

    if (ret == FileStatus::OK) {
        if (new_offset) {
            *new_offset = new_offset_temp;
        }
    } else if (ret <= FileStatus::FATAL) {
        priv->state = FileState::FATAL;
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
 * \return
 *   * #FileStatus::OK if the file size was successfully changed
 *   * #FileStatus::UNSUPPORTED if the handle source does not support truncation
 *   * \<= #FileStatus::WARN if an error occurs
 */
FileStatus File::truncate(uint64_t size)
{
    GET_PIMPL_OR_RETURN(FileStatus::FATAL);
    ENSURE_STATE(FileState::OPENED);

    auto ret = on_truncate(size);

    if (ret <= FileStatus::FATAL) {
        priv->state = FileState::FATAL;
    }

    return ret;
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
 * \brief Get error code for a failed operation.
 *
 * \note The return value is undefined if an operation did not fail.
 *
 * \return Error code for failed operation. If \>= 0, then the file is one of
 *         the FileError entries. If \< 0, then the error code is
 *         implementation-defined (usually `-errno` or `-GetLastError()`).
 */
int File::error()
{
    GET_PIMPL_OR_RETURN(0);

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
 * \param error_code Error code
 * \param fmt `printf()`-style format string
 * \param ... `printf()`-style format arguments
 *
 * \return FileStatus::OK if the error was successfully set or
 *         FileStatus::FAILED if an error occured
 */
FileStatus File::set_error(int error_code, const char *fmt, ...)
{
    FileStatus ret;
    va_list ap;

    va_start(ap, fmt);
    ret = set_error_v(error_code, fmt, ap);
    va_end(ap);

    return ret;
}

/*!
 * \brief Set error string for a failed operation.
 *
 * \sa File::set_error()
 *
 * \param error_code Error code
 * \param fmt `printf()`-style format string
 * \param ap `printf()`-style format arguments as a va_list
 *
 * \return FileStatus::OK if the error was successfully set or
 *         FileStatus::FAILED if an error occured
 */
FileStatus File::set_error_v(int error_code, const char *fmt, va_list ap)
{
    GET_PIMPL_OR_RETURN(FileStatus::FATAL);

    priv->error_code = error_code;
    return format_v(priv->error_string, fmt, ap)
            ? FileStatus::OK : FileStatus::FAILED;
}

/*!
 * \brief File open callback
 *
 * Subclasses should override this method to implement the code needed to open
 * the file.
 *
 * The method should return:
 *
 *   * #FileStatus::OK if the file was successfully opened
 *   * \<= #FileStatus::WARN if an error occurs
 *
 * If this method is not overridden, it will simply return FileStatus::OK.
 *
 * \return #FileStatus::OK
 */
FileStatus File::on_open()
{
    return FileStatus::OK;
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
 *   * #FileStatus::OK if the file was successfully closed
 *   * \<= #FileStatus::WARN if an error occurs
 *
 * \note Regardless of the return value, the file handle will be considered as
 *       closed and the file handle will allow opening another file.
 *
 * It is guaranteed that no other callbacks will be called, except for
 * on_open(), after this method returns.
 *
 * If this method is not overridden, it will simply return FileStatus::OK.
 *
 * \return #FileStatus::OK
 */
FileStatus File::on_close()
{
    return FileStatus::OK;
}

/*!
 * \brief File read callback
 *
 * Subclasses should override this method to implement the code needed to read
 * from the file.
 *
 * This method should return:
 *
 *   * #FileStatus::OK if some bytes were read or EOF is reached
 *   * #FileStatus::RETRY if the same operation should be reattempted
 *   * #FileStatus::UNSUPPORTED if the file does not support reading
 *   * \<= #FileStatus::WARN if an error occurs
 *
 * If this method is not overridden, it will simply return
 * FileStatus::UNSUPPORTED.
 *
 * \param[out] buf Buffer to read into
 * \param[in] size Buffer size
 * \param[out] bytes_read Output number of bytes that were read. 0 indicates end
 *                        of file. This parameter is guaranteed to be non-NULL.
 *
 * \return #FileStatus::UNSUPPORTED
 */
FileStatus File::on_read(void *buf, size_t size, size_t *bytes_read)
{
    (void) buf;
    (void) size;
    (void) bytes_read;

    set_error(FileError::UNSUPPORTED,
              "%s: Read callback not supported", __func__);
    return FileStatus::UNSUPPORTED;
}

/*!
 * \brief File write callback
 *
 * Subclasses should override this method to implement the code needed to write
 * to the file.
 *
 * This method should return:
 *
 *   * #FileStatus::OK if some bytes were written
 *   * #FileStatus::RETRY if the same operation should be reattempted
 *   * #FileStatus::UNSUPPORTED if the file does not support writing
 *   * \<= #FileStatus::WARN if an error occurs
 *
 * If this method is not overridden, it will simply return
 * FileStatus::UNSUPPORTED.
 *
 * \param[in] buf Buffer to write from
 * \param[in] size Buffer size
 * \param[out] bytes_written Output number of bytes that were written. This
 *                           parameter is guaranteed to be non-NULL.
 *
 * \return #FileStatus::UNSUPPORTED
 */
FileStatus File::on_write(const void *buf, size_t size, size_t *bytes_written)
{
    (void) buf;
    (void) size;
    (void) bytes_written;

    set_error(FileError::UNSUPPORTED,
              "%s: Write callback not supported", __func__);
    return FileStatus::UNSUPPORTED;
}

/*!
 * \brief File seek callback
 *
 * Subclasses should override this method to implement the code needed to
 * perform seeking on the file.
 *
 * This method should return:
 *
 *   * #FileStatus::OK if the file position was successfully set
 *   * #FileStatus::UNSUPPORTED if the file does not support seeking
 *   * \<= #FileStatus::WARN if an error occurs
 *
 * If this method is not overridden, it will simply return
 * FileStatus::UNSUPPORTED.
 *
 * \param[in] offset File position offset
 * \param[in] whence SEEK_SET, SEEK_CUR, or SEEK_END from `stdio.h`
 * \param[out] new_offset Output new file offset. This parameter is guaranteed
 *                        to be non-NULL.
 *
 * \return #FileStatus::UNSUPPORTED
 */
FileStatus File::on_seek(int64_t offset, int whence, uint64_t *new_offset)
{
    (void) offset;
    (void) whence;
    (void) new_offset;

    set_error(FileError::UNSUPPORTED,
              "%s: Seek callback not supported", __func__);
    return FileStatus::UNSUPPORTED;
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
 *   * #FileStatus::OK if the file size was successfully changed
 *   * #FileStatus::UNSUPPORTED if the handle source does not support
 *     truncation
 *   * \<= #FileStatus::WARN if an error occurs
 *
 * If this method is not overridden, it will simply return
 * FileStatus::UNSUPPORTED.
 *
 * \param size New size of file
 *
 * \return #FileStatus::UNSUPPORTED
 */
FileStatus File::on_truncate(uint64_t size)
{
    (void) size;

    set_error(FileError::UNSUPPORTED,
              "%s: Truncate callback not supported", __func__);
    return FileStatus::UNSUPPORTED;
}

}
