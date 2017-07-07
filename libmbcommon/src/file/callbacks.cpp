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

#include "mbcommon/file/callbacks.h"

#include "mbcommon/file/callbacks_p.h"

/*!
 * \file mbcommon/file/callbacks.h
 * \brief Open file with callbacks
 */

// Typedefs documentation

namespace mb
{

/*!
 * \typedef CallbackFile::OpenCb
 *
 * \brief File open callback
 *
 * \note If a failure error code is returned. The #CallbackFile::CloseCb
 *       callback, if registered, will be called to clean up the resources.
 *
 * \param file File handle
 *
 * \return
 *   * Return #FileStatus::OK if the file was successfully opened
 *   * Return \<= #FileStatus::WARN if an error occurs
 */

/*!
 * \typedef CallbackFile::CloseCb
 *
 * \brief File close callback
 *
 * This callback, if registered, will be called once and only once once to clean
 * up the resources, regardless of the current state. In other words, this
 * callback will be called even if a function returns #FileStatus::FATAL. If any
 * memory, file handles, or other resources need to be freed, this callback is
 * the place to do so.
 *
 * It is guaranteed that no further callbacks will be invoked after this
 * callback executes.
 *
 * \param file File handle
 *
 * \return
 *   * Return #FileStatus::OK if the file was successfully closed
 *   * Return \<= #FileStatus::WARN if an error occurs
 */

/*!
 * \typedef CallbackFile::ReadCb
 *
 * \brief File read callback
 *
 * \param[in] file File handle
 * \param[out] buf Buffer to read into
 * \param[in] size Buffer size
 * \param[out] bytes_read Output number of bytes that were read. 0 indicates end
 *                        of file. This parameter is guaranteed to be non-NULL.
 *
 * \return
 *   * Return #FileStatus::OK if some bytes were read or EOF is reached
 *   * Return #FileStatus::RETRY if the same operation should be reattempted
 *   * Return #FileStatus::UNSUPPORTED if the file does not support reading
 *     (Not registering a read callback has the same effect.)
 *   * Return \<= #FileStatus::WARN if an error occurs
 */

/*!
 * \typedef CallbackFile::WriteCb
 *
 * \brief File write callback
 *
 * \param[in] file File handle
 * \param[in] buf Buffer to write from
 * \param[in] size Buffer size
 * \param[out] bytes_written Output number of bytes that were written. This
 *                           parameter is guaranteed to be non-NULL.
 *
 * \return
 *   * Return #FileStatus::OK if some bytes were written
 *   * Return #FileStatus::RETRY if the same operation should be reattempted
 *   * Return #FileStatus::UNSUPPORTED if the file does not support writing
 *     (Not registering a read callback has the same effect.)
 *   * Return \<= #FileStatus::WARN if an error occurs
 */

/*!
 * \typedef CallbackFile::SeekCb
 *
 * \brief File seek callback
 *
 * \param[in] file File handle
 * \param[in] offset File position offset
 * \param[in] whence SEEK_SET, SEEK_CUR, or SEEK_END from `stdio.h`
 * \param[out] new_offset Output new file offset. This parameter is guaranteed
 *                        to be non-NULL.
 *
 * \return
 *   * Return #FileStatus::OK if the file position was successfully set
 *   * Return #FileStatus::UNSUPPORTED if the file does not support seeking
 *     (Not registering a seek callback has the same effect.)
 *   * Return \<= #FileStatus::WARN if an error occurs
 */

/*!
 * \typedef CallbackFile::TruncateCb
 *
 * \brief File truncate callback
 *
 * \note This callback must *not* change the file position.
 *
 * \param file File handle
 * \param size New size of file
 *
 * \return
 *   * Return #FileStatus::OK if the file size was successfully changed
 *   * Return #FileStatus::UNSUPPORTED if the handle source does not support
 *     truncation (Not registering a truncate callback has the same effect.)
 *   * Return \<= #FileStatus::WARN if an error occurs
 */

/*! \cond INTERNAL */

CallbackFilePrivate::CallbackFilePrivate()
{
    clear();
}

CallbackFilePrivate::~CallbackFilePrivate()
{
}

void CallbackFilePrivate::clear()
{
    open_cb = nullptr;
    close_cb = nullptr;
    read_cb = nullptr;
    write_cb = nullptr;
    seek_cb = nullptr;
    truncate_cb = nullptr;
    userdata = nullptr;
}

/*! \endcond */

/*!
 * \class CallbackFile
 *
 * \brief Open file with C-style callbacks.
 */

/*!
 * \brief Construct unbound CallbackFile.
 *
 * The File handle will not be bound to any file. The open(OpenCb, CloseCb,
 * ReadCb, WriteCb, SeekCb, TruncateCb, void *) function will need to be called
 * to open a file.
 */
CallbackFile::CallbackFile()
    : CallbackFile(new CallbackFilePrivate())
{
}

/*!
 * \brief Open File handle with callbacks.
 *
 * Construct the file handle and open the file. Use is_open() to check if the
 * file was successfully opened.
 *
 * \sa open(OpenCb, CloseCb, ReadCb, WriteCb, SeekCb, TruncateCb, void *)
 *
 * \param open_cb File open callback
 * \param close_cb File close callback
 * \param read_cb File read callback
 * \param write_cb File write callback
 * \param seek_cb File seek callback
 * \param truncate_cb File truncate callback
 * \param userdata Data pointer to pass to callbacks
 */
CallbackFile::CallbackFile(OpenCb open_cb,
                           CloseCb close_cb,
                           ReadCb read_cb,
                           WriteCb write_cb,
                           SeekCb seek_cb,
                           TruncateCb truncate_cb,
                           void *userdata)
    : CallbackFile(new CallbackFilePrivate(), open_cb, close_cb, read_cb,
                   write_cb, seek_cb, truncate_cb, userdata)
{
}

/*! \cond INTERNAL */

CallbackFile::CallbackFile(CallbackFilePrivate *priv)
    : File(priv)
{
}

CallbackFile::CallbackFile(CallbackFilePrivate *priv,
                           OpenCb open_cb,
                           CloseCb close_cb,
                           ReadCb read_cb,
                           WriteCb write_cb,
                           SeekCb seek_cb,
                           TruncateCb truncate_cb,
                           void *userdata)
    : File(priv)
{
    open(open_cb, close_cb, read_cb, write_cb, seek_cb, truncate_cb, userdata);
}

/*! \endcond */

CallbackFile::~CallbackFile()
{
    close();
}

/*!
 * \brief Open file with callbacks.
 *
 * If \p open_cb is provided and returns a failure code, then \p close_cb, if
 * provided, will be called to clean up any resources.
 *
 * \param open_cb File open callback
 * \param close_cb File close callback
 * \param read_cb File read callback
 * \param write_cb File write callback
 * \param seek_cb File seek callback
 * \param truncate_cb File truncate callback
 * \param userdata Data pointer to pass to callbacks
 */
FileStatus CallbackFile::open(OpenCb open_cb,
                              CloseCb close_cb,
                              ReadCb read_cb,
                              WriteCb write_cb,
                              SeekCb seek_cb,
                              TruncateCb truncate_cb,
                              void *userdata)
{
    MB_PRIVATE(CallbackFile);

    // pimpl can be NULL if the handle was moved. File::open() will fail
    // appropriately in that case.
    if (priv) {
        priv->open_cb = open_cb;
        priv->close_cb = close_cb;
        priv->read_cb = read_cb;
        priv->write_cb = write_cb;
        priv->seek_cb = seek_cb;
        priv->truncate_cb = truncate_cb;
        priv->userdata = userdata;
    }

    return File::open();
}

FileStatus CallbackFile::on_open()
{
    MB_PRIVATE(CallbackFile);

    if (priv->open_cb) {
        return priv->open_cb(*this, priv->userdata);
    } else {
        return File::on_open();
    }
}

FileStatus CallbackFile::on_close()
{
    MB_PRIVATE(CallbackFile);

    FileStatus ret;

    if (priv->close_cb) {
        ret = priv->close_cb(*this, priv->userdata);
    } else {
        ret = File::on_close();
    }

    // Reset to allow opening another file
    priv->clear();

    return ret;
}

FileStatus CallbackFile::on_read(void *buf, size_t size, size_t *bytes_read)
{
    MB_PRIVATE(CallbackFile);

    if (priv->read_cb) {
        return priv->read_cb(*this, priv->userdata, buf, size, bytes_read);
    } else {
        return File::on_read(buf, size, bytes_read);
    }
}

FileStatus CallbackFile::on_write(const void *buf, size_t size,
                                  size_t *bytes_written)
{
    MB_PRIVATE(CallbackFile);

    if (priv->write_cb) {
        return priv->write_cb(*this, priv->userdata, buf, size, bytes_written);
    } else {
        return File::on_write(buf, size, bytes_written);
    }
}

FileStatus CallbackFile::on_seek(int64_t offset, int whence,
                                 uint64_t *new_offset)
{
    MB_PRIVATE(CallbackFile);

    if (priv->seek_cb) {
        return priv->seek_cb(*this, priv->userdata, offset, whence, new_offset);
    } else {
        return File::on_seek(offset, whence, new_offset);
    }
}

FileStatus CallbackFile::on_truncate(uint64_t size)
{
    MB_PRIVATE(CallbackFile);

    if (priv->truncate_cb) {
        return priv->truncate_cb(*this, priv->userdata, size);
    } else {
        return File::on_truncate(size);
    }
}

}
