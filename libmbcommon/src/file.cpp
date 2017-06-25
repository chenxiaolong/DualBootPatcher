/*
 * Copyright (C) 2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include <cerrno>
#include <cstdio>
#include <cstdlib>

#include "mbcommon/file_p.h"
#include "mbcommon/string.h"

#define ENSURE_STATE(HANDLE, STATES) \
    do { \
        if (!((HANDLE)->state & (STATES))) { \
            mb_file_set_error((HANDLE), MB_FILE_ERROR_PROGRAMMER_ERROR, \
                              "%s: Invalid state: "\
                              "expected 0x%hx, actual: 0x%hx", \
                              __func__, (STATES), (HANDLE)->state); \
            (HANDLE)->state = MbFileState::FATAL; \
            return MB_FILE_FATAL; \
        } \
    } while (0)

// File documentation

/*!
 * \file mbcommon/file.h
 * \brief File abstraction API
 */

// Types documentation

/*!
 * \struct MbFile
 *
 * \brief Opaque handle for mb_file_* functions.
 */

// Return values documentation

/*!
 * \enum MbFileRet
 *
 * \brief Possible return values for functions.
 */

/*!
 * \var MbFileRet::MB_FILE_OK
 *
 * \brief Success error code
 *
 * Success error code.
 */

/*!
 * \var MbFileRet::MB_FILE_RETRY
 *
 * \brief Reattempt operation
 *
 * The operation should be reattempted.
 */

/*!
 * \var MbFileRet::MB_FILE_WARN
 *
 * \brief Warning
 *
 * The operation raised a warning. The MbFile handle can still be used although
 * the functionality may be degraded.
 */

/*!
 * \var MbFileRet:: MB_FILE_FAILED
 *
 * \brief Non-fatal error
 *
 * The operation failed non-fatally. The MbFile handle can still be used for
 * further operations.
 */

/*!
 * \var MbFileRet::MB_FILE_FATAL
 *
 * \brief Fatal error
 *
 * The operation failed fatally. The MbFile handle can no longer be used and
 * should be freed with mb_file_free().
 */

/*!
 * \var MbFileRet::MB_FILE_UNSUPPORTED
 *
 * \brief Operation not supported
 *
 * The operation is not supported.
 */

// Error codes documentation

/*!
 * \enum MbFileError
 *
 * \brief Possible error codes.
 */

/*!
 * \var MbFileError::MB_FILE_ERROR_NONE
 *
 * \brief No error
 */

/*!
 * \var MbFileError::MB_FILE_ERROR_INVALID_ARGUMENT
 *
 * \brief An invalid argument was provided
 */

/*!
 * \var MbFileError::MB_FILE_ERROR_UNSUPPORTED
 *
 * \brief The operation is not supported
 */

/*!
 * \var MbFileError::MB_FILE_ERROR_PROGRAMMER_ERROR
 *
 * \brief The function were called in an invalid state
 */

/*!
 * \var MbFileError::MB_FILE_ERROR_INTERNAL_ERROR
 *
 * \brief Internal error in the library
 */

// Typedefs documentation

/*!
 * \typedef MbFileOpenCb
 *
 * \brief File open callback
 *
 * \note If a failure error code is returned. The #MbFileCloseCb callback,
 *       if registered, will be called to clean up the resources.
 *
 * \param file MbFile handle
 *
 * \return
 *   * Return #MB_FILE_OK if the file was successfully opened
 *   * Return \<= #MB_FILE_WARN if an error occurs
 */

/*!
 * \typedef MbFileCloseCb
 *
 * \brief File close callback
 *
 * This callback, if registered, will be called once and only once once to clean
 * up the resources, regardless of the current state. In other words, this
 * callback will be called even if a function returns #MB_FILE_FATAL. If any
 * memory, file handles, or other resources need to be freed, this callback is
 * the place to do so.
 *
 * It is guaranteed that no further callbacks will be invoked after this
 * callback executes.
 *
 * \param file MbFile handle
 *
 * \return
 *   * Return #MB_FILE_OK if the file was successfully closed
 *   * Return \<= #MB_FILE_WARN if an error occurs
 */

/*!
 * \typedef MbFileReadCb
 *
 * \brief File read callback
 *
 * \param[in] file MbFile handle
 * \param[out] buf Buffer to read into
 * \param[in] size Buffer size
 * \param[out] bytes_read Output number of bytes that were read. 0 indicates end
 *                        of file. This parameter is guaranteed to be non-NULL.
 *
 * \return
 *   * Return #MB_FILE_OK if some bytes were read or EOF is reached
 *   * Return #MB_FILE_RETRY if the same operation should be reattempted
 *   * Return #MB_FILE_UNSUPPORTED if the file does not support reading
 *     (Not registering a read callback has the same effect.)
 *   * Return \<= #MB_FILE_WARN if an error occurs
 */

/*!
 * \typedef MbFileWriteCb
 *
 * \brief File write callback
 *
 * \param[in] file MbFile handle
 * \param[in] buf Buffer to write from
 * \param[in] size Buffer size
 * \param[out] bytes_written Output number of bytes that were written. This
 *                           parameter is guaranteed to be non-NULL.
 *
 * \return
 *   * Return #MB_FILE_OK if some bytes were written
 *   * Return #MB_FILE_RETRY if the same operation should be reattempted
 *   * Return #MB_FILE_UNSUPPORTED if the file does not support writing
 *     (Not registering a read callback has the same effect.)
 *   * Return \<= #MB_FILE_WARN if an error occurs
 */

/*!
 * \typedef MbFileSeekCb
 *
 * \brief File seek callback
 *
 * \param[in] file MbFile handle
 * \param[in] offset File position offset
 * \param[in] whence SEEK_SET, SEEK_CUR, or SEEK_END from `stdio.h`
 * \param[out] new_offset Output new file offset. This parameter is guaranteed
 *                        to be non-NULL.
 *
 * \return
 *   * Return #MB_FILE_OK if the file position was successfully set
 *   * Return #MB_FILE_UNSUPPORTED if the file does not support seeking
 *     (Not registering a seek callback has the same effect.)
 *   * Return \<= #MB_FILE_WARN if an error occurs
 */

/*!
 * \typedef MbFileTruncateCb
 *
 * \brief File truncate callback
 *
 * \note This callback must *not* change the file position.
 *
 * \param file MbFile handle
 * \param size New size of file
 *
 * \return
 *   * Return #MB_FILE_OK if the file size was successfully changed
 *   * Return #MB_FILE_UNSUPPORTED if the handle source does not support
 *     truncation (Not registering a truncate callback has the same effect.)
 *   * Return \<= #MB_FILE_WARN if an error occurs
 */

MB_BEGIN_C_DECLS

/*!
 * \brief Allocate new MbFile handle.
 *
 * \return New MbFile handle or NULL if memory could not be allocated. If the
 *         function fails, `errno` will be set accordingly.
 */
struct MbFile * mb_file_new()
{
    struct MbFile *file = static_cast<struct MbFile *>(
            calloc(1, sizeof(struct MbFile)));
    if (file) {
        file->state = MbFileState::NEW;
    }
    return file;
}

/*!
 * \brief Free an MbFile handle.
 *
 * If the handle has not been closed, it will be closed and the result of
 * mb_file_close() will be returned. Otherwise, #MB_FILE_OK will be returned.
 * Regardless of the return value, the handle will always be freed and should
 * no longer be used.
 *
 * \param file MbFile handle
 * \return The result of mb_file_close() if the file has not been closed;
 *         otherwise, #MB_FILE_OK.
 */
int mb_file_free(struct MbFile *file)
{
    int ret = MB_FILE_OK;

    if (file) {
        if (file->state != MbFileState::CLOSED) {
            ret = mb_file_close(file);
        }

        free(file->error_string);
        free(file);
    }

    return ret;
}

/*!
 * \brief Set the file open callback for an MbFile handle.
 *
 * \param file MbFile handle
 * \param open_cb File open callback
 *
 * \return
 *   * #MB_FILE_OK if the callback was successfully set
 *   * #MB_FILE_FATAL if the file has already been opened
 */
int mb_file_set_open_callback(struct MbFile *file, MbFileOpenCb open_cb)
{
    ENSURE_STATE(file, MbFileState::NEW);
    file->open_cb = open_cb;
    return MB_FILE_OK;
}

/*!
 * \brief Set the file close callback for an MbFile handle.
 *
 * \param file MbFile handle
 * \param close_cb File close callback
 *
 * \return
 *   * #MB_FILE_OK if the callback was successfully set
 *   * #MB_FILE_FATAL if the file has already been opened
 */
int mb_file_set_close_callback(struct MbFile *file, MbFileCloseCb close_cb)
{
    ENSURE_STATE(file, MbFileState::NEW);
    file->close_cb = close_cb;
    return MB_FILE_OK;
}

/*!
 * \brief Set the file read callback for an MbFile handle.
 *
 * \param file MbFile handle
 * \param read_cb File read callback
 *
 * \return
 *   * #MB_FILE_OK if the callback was successfully set
 *   * #MB_FILE_FATAL if the file has already been opened
 */
int mb_file_set_read_callback(struct MbFile *file, MbFileReadCb read_cb)
{
    ENSURE_STATE(file, MbFileState::NEW);
    file->read_cb = read_cb;
    return MB_FILE_OK;
}

/*!
 * \brief Set the file write callback for an MbFile handle.
 *
 * \param file MbFile handle
 * \param write_cb File write callback
 *
 * \return
 *   * #MB_FILE_OK if the callback was successfully set
 *   * #MB_FILE_FATAL if the file has already been opened
 */
int mb_file_set_write_callback(struct MbFile *file, MbFileWriteCb write_cb)
{
    ENSURE_STATE(file, MbFileState::NEW);
    file->write_cb = write_cb;
    return MB_FILE_OK;
}

/*!
 * \brief Set the file seek callback for an MbFile handle.
 *
 * \param file MbFile handle
 * \param seek_cb File seek callback
 *
 * \return
 *   * #MB_FILE_OK if the callback was successfully set
 *   * #MB_FILE_FATAL if the file has already been opened
 */
int mb_file_set_seek_callback(struct MbFile *file, MbFileSeekCb seek_cb)
{
    ENSURE_STATE(file, MbFileState::NEW);
    file->seek_cb = seek_cb;
    return MB_FILE_OK;
}

/*!
 * \brief Set the file truncate callback for an MbFile handle.
 *
 * \param file MbFile handle
 * \param truncate_cb File truncate callback
 *
 * \return
 *   * #MB_FILE_OK if the callback was successfully set
 *   * #MB_FILE_FATAL if the file has already been opened
 */
int mb_file_set_truncate_callback(struct MbFile *file,
                                  MbFileTruncateCb truncate_cb)
{
    ENSURE_STATE(file, MbFileState::NEW);
    file->truncate_cb = truncate_cb;
    return MB_FILE_OK;
}

/*!
 * \brief Set the data to provide to callbacks for an MbFile handle.
 *
 * \param file MbFile handle
 * \param userdata User-provided data pointer for callbacks
 *
 * \return
 *   * #MB_FILE_OK if the userdata was successfully set
 *   * #MB_FILE_FATAL if the file has already been opened
 */
int mb_file_set_callback_data(struct MbFile *file, void *userdata)
{
    ENSURE_STATE(file, MbFileState::NEW);
    file->cb_userdata = userdata;
    return MB_FILE_OK;
}

/*!
 * \brief Open an MbFile handle.
 *
 * Once the handle has been opened, the file operation functions, such as
 * mb_file_read(), are available to use. It will no longer be possible to set
 * the callback functions for this handle.
 *
 * \param file MbFile handle
 *
 * \return
 *   * #MB_FILE_OK if there is no file open handle or if the file open handle
 *     succeeds
 *   * \<= #MB_FILE_WARN if an error occurs
 */
int mb_file_open(struct MbFile *file)
{
    int ret = MB_FILE_OK;

    ENSURE_STATE(file, MbFileState::NEW);

    if (file->open_cb) {
        ret = file->open_cb(file, file->cb_userdata);
    }
    if (ret == MB_FILE_OK) {
        file->state = MbFileState::OPENED;
    } else if (ret <= MB_FILE_FATAL) {
        file->state = MbFileState::FATAL;
    }

    // If the file was not successfully opened, then close it
    if (ret != MB_FILE_OK && file->close_cb) {
        file->close_cb(file, file->cb_userdata);
    }

    return ret;
}

/*!
 * \brief Close an MbFile handle.
 *
 * This function will close an MbFile handle if it is open. Regardless of the
 * return value, the handle is closed and can no longer be used for further
 * operations. It should be freed with mb_file_free().
 *
 * \param file MbFile handle
 *
 * \return
 *   * #MB_FILE_OK if no error was encountered when closing the handle.
 *   * \<= #MB_FILE_WARN if the handle is opened and an error occurs while
 *     closing the file
 */
int mb_file_close(struct MbFile *file)
{
    int ret = MB_FILE_OK;

    // Avoid double-closing or closing nothing
    if (!(file->state & (MbFileState::CLOSED | MbFileState::NEW))) {
        if (file->close_cb) {
            ret = file->close_cb(file, file->cb_userdata);
        }

        // Don't change state to MbFileState::FATAL if MB_FILE_FATAL is
        // returned. Otherwise, we risk double-closing the file. CLOSED and
        // FATAL are the same anyway, aside from the fact that files can be
        // closed in the latter state.
    }

    file->state = MbFileState::CLOSED;

    return ret;
}

/*!
 * \brief Read from an MbFile handle.
 *
 * Example usage:
 *
 *     char buf[10240];
 *     int ret;
 *     size_t n;
 *
 *     while ((ret = mb_file_read(file, buf, sizeof(buf), &n)) == MB_FILE_OK
 *             && n >= 0) {
 *         fwrite(buf, 1, n, stdout);
 *     }
 *
 *     if (ret != MB_FILE_OK) {
 *         printf("Failed to read file: %s\n", mb_file_error_string(file));
 *     }
 *
 * \param[in] file MbFile handle
 * \param[out] buf Buffer to read into
 * \param[in] size Buffer size
 * \param[out] bytes_read Output number of bytes that were read. 0 indicates end
 *                        of file. This parameter cannot be NULL.
 *
 * \return
 *   * #MB_FILE_OK if some bytes were read or EOF is reached
 *   * #MB_FILE_RETRY if the same operation should be reattempted
 *   * #MB_FILE_UNSUPPORTED if the handle source does not support reading
 *   * \<= #MB_FILE_WARN if an error occurs
 */
int mb_file_read(struct MbFile *file, void *buf, size_t size,
                 size_t *bytes_read)
{
    int ret = MB_FILE_UNSUPPORTED;

    ENSURE_STATE(file, MbFileState::OPENED);

    if (!bytes_read) {
        mb_file_set_error(file, MB_FILE_ERROR_PROGRAMMER_ERROR,
                          "%s: bytes_read is NULL",
                          __func__);
        ret = MB_FILE_FATAL;
    } else if (file->read_cb) {
        ret = file->read_cb(file, file->cb_userdata, buf, size, bytes_read);
    } else {
        mb_file_set_error(file, MB_FILE_ERROR_UNSUPPORTED,
                          "%s: No read callback registered",
                          __func__);
    }
    if (ret <= MB_FILE_FATAL) {
        file->state = MbFileState::FATAL;
    }

    return ret;
}

/*!
 * \brief Write to an MbFile handle.
 *
 * Example usage:
 *
 *     size_t n;
 *
 *     if (mb_file_write(file, buf, sizeof(buf), &bytesWritten)) {
 *         printf("Failed to write file: %s\n", mb_file_error_string(file));
 *     }
 *
 * \param[in] file MbFile handle
 * \param[in] buf Buffer to write from
 * \param[in] size Buffer size
 * \param[out] bytes_written Output number of bytes that were written. This
 *                           parameter cannot be NULL.
 *
 * \return
 *   * #MB_FILE_OK if some bytes were written
 *   * #MB_FILE_RETRY if the same operation should be reattempted
 *   * #MB_FILE_UNSUPPORTED if the handle source does not support writing
 *   * \<= #MB_FILE_WARN if an error occurs
 */
int mb_file_write(struct MbFile *file, const void *buf, size_t size,
                  size_t *bytes_written)
{
    int ret = MB_FILE_UNSUPPORTED;

    ENSURE_STATE(file, MbFileState::OPENED);

    if (!bytes_written) {
        mb_file_set_error(file, MB_FILE_ERROR_PROGRAMMER_ERROR,
                          "%s: bytes_written is NULL",
                          __func__);
        ret = MB_FILE_FATAL;
    } else if (file->write_cb) {
        ret = file->write_cb(file, file->cb_userdata, buf, size, bytes_written);
    } else {
        mb_file_set_error(file, MB_FILE_ERROR_UNSUPPORTED,
                          "%s: No write callback registered",
                          __func__);
    }
    if (ret <= MB_FILE_FATAL) {
        file->state = MbFileState::FATAL;
    }

    return ret;
}

/*!
 * \brief Set file position of an MbFile handle.
 *
 * \param[in] file MbFile handle
 * \param[in] offset File position offset
 * \param[in] whence SEEK_SET, SEEK_CUR, or SEEK_END from `stdio.h`
 * \param[out] new_offset Output new file offset. This parameter can be NULL.
 *
 * \return
 *   * #MB_FILE_OK if the file position was successfully set
 *   * #MB_FILE_UNSUPPORTED if the handle source does not support seeking
 *   * \<= #MB_FILE_WARN if an error occurs
 */
int mb_file_seek(struct MbFile *file, int64_t offset, int whence,
                 uint64_t *new_offset)
{
    int ret = MB_FILE_UNSUPPORTED;
    uint64_t new_offset_temp;

    ENSURE_STATE(file, MbFileState::OPENED);

    if (file->seek_cb) {
        ret = file->seek_cb(file, file->cb_userdata, offset, whence,
                            &new_offset_temp);
    } else {
        mb_file_set_error(file, MB_FILE_ERROR_UNSUPPORTED,
                          "%s: No seek callback registered",
                          __func__);
    }
    if (ret == MB_FILE_OK) {
        if (new_offset) {
            *new_offset = new_offset_temp;
        }
    } else if (ret <= MB_FILE_FATAL) {
        file->state = MbFileState::FATAL;
    }

    return ret;
}

/*!
 * \brief Truncate or extend file backed by an MbFile handle.
 *
 * \note The file position is *not* changed after a successful call of this
 *       function. The size of the file may increase if the file position is
 *       larger than the truncated file size and mb_file_write() is called.
 *
 * \param file MbFile handle
 * \param size New size of file
 *
 * \return
 *   * #MB_FILE_OK if the file size was successfully changed
 *   * #MB_FILE_UNSUPPORTED if the handle source does not support truncation
 *   * \<= #MB_FILE_WARN if an error occurs
 */
int mb_file_truncate(struct MbFile *file, uint64_t size)
{
    int ret = MB_FILE_UNSUPPORTED;

    ENSURE_STATE(file, MbFileState::OPENED);

    if (file->truncate_cb) {
        ret = file->truncate_cb(file, file->cb_userdata, size);
    } else {
        mb_file_set_error(file, MB_FILE_ERROR_UNSUPPORTED,
                          "%s: No truncate callback registered",
                          __func__);
    }
    if (ret <= MB_FILE_FATAL) {
        file->state = MbFileState::FATAL;
    }

    return ret;
}

/*!
 * \brief Get error code for a failed operation.
 *
 * \note The return value is undefined if an operation did not fail.
 *
 * \param file MbFile handle
 *
 * \return Error code for failed operation. If \>= 0, then the file is one of
 *         the MbFileError entries. If \< 0, then the error code is
 *         implementation-defined (usually `-errno` or `-GetLastError()`).
 */
int mb_file_error(struct MbFile *file)
{
    return file->error_code;
}

/*!
 * \brief Get error string for a failed operation.
 *
 * \note The return value is undefined if an operation did not fail.
 *
 * \param file MbFile handle
 *
 * \return Error string for failed operation. The string contents may be
 *         undefined, but will never be NULL or an invalid string.
 */
const char * mb_file_error_string(struct MbFile *file)
{
    return file->error_string ? file->error_string : "";
}

/*!
 * \brief Set error string for a failed operation.
 *
 * \sa mb_file_set_error_v()
 *
 * \param file MbFile handle
 * \param error_code Error code
 * \param fmt `printf()`-style format string
 * \param ... `printf()`-style format arguments
 *
 * \return MB_FILE_OK if the error was successfully set or MB_FILE_FAILED if
 *         an error occured
 */
int mb_file_set_error(struct MbFile *file, int error_code,
                      const char *fmt, ...)
{
    int ret;
    va_list ap;

    va_start(ap, fmt);
    ret = mb_file_set_error_v(file, error_code, fmt, ap);
    va_end(ap);

    return ret;
}

/*!
 * \brief Set error string for a failed operation.
 *
 * \sa mb_file_set_error()
 *
 * \param file MbFile handle
 * \param error_code Error code
 * \param fmt `printf()`-style format string
 * \param ap `printf()`-style format arguments as a va_list
 *
 * \return MB_FILE_OK if the error was successfully set or MB_FILE_FAILED if
 *         an error occured
 */
int mb_file_set_error_v(struct MbFile *file, int error_code,
                        const char *fmt, va_list ap)
{
    free(file->error_string);

    char *dup = mb_format_v(fmt, ap);
    if (!dup) {
        return MB_FILE_FAILED;
    }

    file->error_code = error_code;
    file->error_string = dup;
    return MB_FILE_OK;
}

MB_END_C_DECLS
